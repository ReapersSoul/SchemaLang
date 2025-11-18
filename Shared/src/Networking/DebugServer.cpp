#include <Networking/DebugServer.hpp>

DebugServer::DebugServer(unsigned short port)
    : port_(port), running_(false)
{
}

DebugServer::~DebugServer()
{
    stop();
}

void DebugServer::start()
{
    if (running_)
        return;
    running_ = true;
    listener_thread_ = std::thread(&DebugServer::listenerLoop, this);
}

void DebugServer::stop()
{
    if (!running_)
        return;
    running_ = false;
    if (listener_thread_.joinable())
    {
        listener_thread_.join();
    }
}

bool DebugServer::isRunning()
{
    return running_;
}

void DebugServer::listenerLoop()
{
    try
    {
        net::io_context ioc{1};

        tcp::acceptor acceptor{ioc, tcp::endpoint{tcp::v4(), port_}};

        while (running_)
        {
            tcp::socket socket{ioc};
            acceptor.accept(socket);

            auto new_session = std::make_shared<session>(std::move(socket), shared_from_this());
            onNewSession_(new_session);
            sessions_.push_back(new_session);
            new_session->start();
        }
    }
    catch (std::exception const &e)
    {
        std::cerr << "Listener error: " << e.what() << std::endl;
    }
}

void DebugServer::join()
{
    if (listener_thread_.joinable()){
        listener_thread_.join();
    }
}

session::session(tcp::socket socket, std::shared_ptr<DebugServer> server)
    : ws_(std::move(socket)), server_(server)
{
}

void session::start()
{
    ws_.accept();
    receive_thread_ = std::thread(&session::receiveLoop, this);
}

void session::stop()
{
    if (receive_thread_.joinable())
    {
        receive_thread_.join();
    }
    ws_.close(websocket::close_code::normal);
}

bool session::isRunning() const
{
    return ws_.is_open();
}

bool session::sendMessage(nlohmann::json message)
{
    try
    {
        std::string msg_str = message.dump();
        ws_.text(true);
        ws_.write(net::buffer(msg_str));
        return true;
    }
    catch (std::exception const &e)
    {
        std::cerr << "Send error: " << e.what() << std::endl;
        return false;
    }
}

bool session::receiveMessage(nlohmann::json &message)
{
    try
    {
        beast::flat_buffer buffer;
        ws_.read(buffer);
        std::string msg_str = beast::buffers_to_string(buffer.data());
        message = nlohmann::json::parse(msg_str);
        return true;
    }
    catch (std::exception const &e)
    {
        std::cerr << "Receive error: " << e.what() << std::endl;
        return false;
    }
}

void session::receiveLoop()
{
    while (isRunning())
    {
        nlohmann::json message;
        if (receiveMessage(message))
        {
            // Process the received message
            onMessage(message);
        }
        else
        {
            break; // Exit loop on receive error
        }
    }
}

void session::onMessage(nlohmann::json message)
{
    /*
    {
        "seq": 1,
        "type": "request",
        "command": "initialize",
        "arguments": {
            "clientID": "your-client-id",
            "clientName": "Your Client Name",
            "adapterID": "your-adapter-id",
            "locale": "en-US",
            "linesStartAt1": true,
            "columnsStartAt1": true,
            "pathFormat": "uri",
            "supportsVariableType": true,
            "supportsVariablePaging": true,
            "supportsRunInTerminalRequest": true,
            "supportsMemoryReferences": true,
            "supportsProgressReporting": true,
            "supportsInvalidatedEvent": true,
            "supportsArgsCanBeInterpretedByShell": true,
            "supportsStartDebuggingRequest": true,
            "supportsTerminateDebuggee": true,
            "supportsSuspendDebuggee": true,
            "supportsDisassembleRequest": true,
            "supportsSteppingGranularity": true,
            "supportsInstructionBreakpoints": true,
            "supportsExceptionInfoRequest": true
        }
    }
    */

    if (message.contains("command") && message["command"] == "initialize")
    {
        nlohmann::json response;
        response["seq"] = 1;
        response["type"] = "response";
        response["request_seq"] = message["seq"];
        response["success"] = true;
        response["command"] = "initialize";
        response["body"] = {
            {"capabilities", {{"supportsConfigurationDoneRequest", true}, {"supportsEvaluateForHovers", true}, {"supportsInstructionBreakpoints", true}, {"supportsVariables", true}, {"supportsSetVariable", true}, {"supportsVariableType", true}, {"supportsVariablePaging", true}}}};
        sendMessage(response);

        // After initialization, send InitializedEvent
        nlohmann::json initialized_event;
        initialized_event["seq"] = 2;
        initialized_event["type"] = "event";
        initialized_event["event"] = "initialized";
        sendMessage(initialized_event);
    }

    // handle configuration
    // set breakpoints
    if (message.contains("command") && message["command"] == "setBreakpoints")
    {
        nlohmann::json response;
        response["seq"] = 3;
        response["type"] = "response";
        response["request_seq"] = message["seq"];
        response["success"] = true;
        response["command"] = "setBreakpoints";
        response["body"] = {
            {"breakpoints", nlohmann::json::array()}};
        sendMessage(response);
    }
}

void session::onTokenParsed(Token token)
{
    for (Breakpoint bp : breakpoints_)
    {
        if (bp.enabled && bp.file_path == token.position.file_path && bp.line_number == token.position.line)
        {
            paused_.store(true, std::memory_order_release);
            paused_.notify_one(); // wakes whoever is waiting
            nlohmann::json event;
            event["seq"] = 4;
            event["type"] = "event";
            event["event"] = "stopped";
            event["body"] = {
                {"reason", "breakpoint"},
                {"threadId", 1},
                {"allThreadsStopped", true}};
            sendMessage(event);
            break;
        }
    }
    paused_.wait(true);
}

void session::beginParseOperation(const std::string &operation)
{
}

void session::endParseOperation()
{
}

void session::begin()
{
}
