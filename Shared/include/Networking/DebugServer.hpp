#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/strand.hpp>
#include <boost/system/error_code.hpp>
#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <deque>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include <Debug.hpp>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

//------------------------------------------------------------------------------

// Report a failure
void
fail(beast::error_code ec, char const* what)
{
    std::cerr << what << ": " << ec.message() << "\n";
}

class session;
class listener;

//------------------------------------------------------------------------------

class DebugServer
{
public:
    class SessionHandle
    {
    public:
        SessionHandle() = default;

        bool isOpen() const;
        void send(nlohmann::json message) const;
        void close(websocket::close_reason reason = websocket::close_reason{websocket::close_code::normal}) const;
        bool waitForMessage(nlohmann::json& message) const;
        bool waitForMessage(nlohmann::json& message, std::chrono::milliseconds timeout) const;

        bool initialize(){
            nlohmann::json init_msg;
            if (!waitForMessage(init_msg)) {
                return false;
            }

            /*
            we expect the following:
            {
                "seq": 1,
                "type": "request",
                "command": "initialize",
                "arguments": {
                    "clientID": "vscode",
                    "adapterID": "my-transpiler-debug",
                    "locale": "en-us",
                    "linesStartAt1": true,
                    "columnsStartAt1": true,
                    "pathFormat": "path",
                    "supportsVariableType": true,
                    "supportsVariablePaging": false,
                    "supportsRunInTerminalRequest": true
                }
            }
            
            and should respond with
            
            {
                "seq": 1,
                "type": "response",
                "request_seq": 1,
                "success": true,
                "command": "initialize",
                "body": {
                    "supportsConfigurationDoneRequest": true,
                    "supportsEvaluateForHovers": true,
                    "supportsSetVariable": false,
                    "supportsConditionalBreakpoints": false,
                    "supportsStepBack": false,
                    "supportsDataBreakpoints": false,
                    "supportsCompletionsRequest": false
                }
            }

            */
            nlohmann::json response;
            response["type"] = "response";
            response["request_seq"] = init_msg["seq"];
            response["success"] = true;
            response["command"] = "initialize";
            response["body"] = {
                {"supportsConfigurationDoneRequest", true},
                {"supportsEvaluateForHovers", true},
                {"supportsSetVariable", false},
                {"supportsConditionalBreakpoints", false},
                {"supportsStepBack", false},
                {"supportsDataBreakpoints", false},
                {"supportsCompletionsRequest", false}
            };
            send(response);
        }

        void OnTokenParsed(Token token){
            
        }
    private:
        explicit SessionHandle(std::weak_ptr<session> session)
            : session_(std::move(session))
        {
        }

        std::weak_ptr<session> session_;

        friend class DebugServer;
        friend class session;
    };

    using OpenHandler = std::function<void(const SessionHandle&)>;
    using MessageHandler = std::function<void(const SessionHandle&, const nlohmann::json&)>;
    using CloseHandler = std::function<void(const SessionHandle&)>;
    using ErrorHandler = std::function<void(const SessionHandle&, beast::error_code, std::string_view)>;

    DebugServer() = default;
    DebugServer(unsigned short port, int threads = 1, std::string address = "127.0.0.1");

    void setAddress(std::string address);
    void setPort(unsigned short port);
    void setThreadCount(int threads);

    void setOnOpen(OpenHandler handler);
    void setOnMessage(MessageHandler handler);
    void setOnClose(CloseHandler handler);
    void setOnError(ErrorHandler handler);

    int run();

private:
    friend class session;
    friend class listener;

    void notifyOpen(const std::shared_ptr<session>& session_ptr);
    void notifyMessage(const std::shared_ptr<session>& session_ptr, const nlohmann::json& message);
    void notifyClose(const std::shared_ptr<session>& session_ptr);
    void notifyError(const std::shared_ptr<session>& session_ptr, beast::error_code ec, std::string_view what);

    SessionHandle makeHandle(const std::shared_ptr<session>& session_ptr) const;

    std::string address_{"127.0.0.1"};
    unsigned short port_{0};
    int thread_count_{1};
    OpenHandler on_open_;
    MessageHandler on_message_;
    CloseHandler on_close_;
    ErrorHandler on_error_;
};

//------------------------------------------------------------------------------

// Handles a single WebSocket connection
class session : public std::enable_shared_from_this<session>
{
public:
    explicit session(tcp::socket&& socket, DebugServer& owner)
        : ws_(std::move(socket))
        , owner_(owner)
    {
    }

    void run()
    {
        net::dispatch(ws_.get_executor(),
            beast::bind_front_handler(
                &session::on_run,
                shared_from_this()));
    }

    DebugServer::SessionHandle makeHandle()
    {
        return DebugServer::SessionHandle{shared_from_this()};
    }

    void send(nlohmann::json message)
    {
        std::string serialized = message.dump();
        net::post(ws_.get_executor(),
            beast::bind_front_handler(
                &session::queue_write,
                shared_from_this(),
                std::move(serialized)));
    }

    bool waitForMessage(nlohmann::json& message);
    bool waitForMessage(nlohmann::json& message, std::chrono::milliseconds timeout);

    void close(websocket::close_reason reason = websocket::close_reason{websocket::close_code::normal})
    {
        net::post(ws_.get_executor(),
            beast::bind_front_handler(
                &session::do_close,
                shared_from_this(),
                std::move(reason)));
    }

    bool isOpen() const
    {
        return ws_.is_open();
    }

private:
    void on_run()
    {
        ws_.set_option(
            websocket::stream_base::timeout::suggested(
                beast::role_type::server));

        ws_.set_option(websocket::stream_base::decorator(
            [](websocket::response_type& res)
            {
                res.set(http::field::server,
                    std::string(BOOST_BEAST_VERSION_STRING) +
                        " websocket-server-async");
            }));

        ws_.async_accept(
            beast::bind_front_handler(
                &session::on_accept,
                shared_from_this()));
    }

    void on_accept(beast::error_code ec)
    {
        if (ec)
        {
            reportError(ec, "accept");
            notifyCloseOnce();
            return;
        }

        owner_.notifyOpen(shared_from_this());
        do_read();
    }

    void do_read()
    {
        ws_.async_read(
            buffer_,
            beast::bind_front_handler(
                &session::on_read,
                shared_from_this()));
    }

    void on_read(beast::error_code ec, std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        if (ec == websocket::error::closed)
        {
            notifyCloseOnce();
            return;
        }

        if (ec)
        {
            reportError(ec, "read");
            notifyCloseOnce();
            return;
        }

        std::string message = beast::buffers_to_string(buffer_.data());
        buffer_.consume(buffer_.size());

        nlohmann::json parsed;
        try
        {
            parsed = nlohmann::json::parse(message);
        }
        catch (const nlohmann::json::parse_error& ex)
        {
            auto ec = boost::system::errc::make_error_code(boost::system::errc::invalid_argument);
            owner_.notifyError(shared_from_this(), ec, ex.what());
            do_read();
            return;
        }

        {
            std::lock_guard<std::mutex> lock(message_mutex_);
            message_queue_.push_back(parsed);
        }
        message_cv_.notify_one();

        owner_.notifyMessage(shared_from_this(), parsed);

        do_read();
    }

    void queue_write(std::string message)
    {
        if (!ws_.is_open())
        {
            return;
        }

        write_queue_.push_back(std::move(message));
        if (write_in_progress_)
        {
            return;
        }

        write_in_progress_ = true;
        ws_.text(true);
        ws_.async_write(
            net::buffer(write_queue_.front()),
            beast::bind_front_handler(
                &session::on_write,
                shared_from_this()));
    }

    void on_write(beast::error_code ec, std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        if (ec)
        {
            write_queue_.clear();
            write_in_progress_ = false;
            reportError(ec, "write");
            notifyCloseOnce();
            return;
        }

        write_queue_.pop_front();

        if (!write_queue_.empty())
        {
            ws_.async_write(
                net::buffer(write_queue_.front()),
                beast::bind_front_handler(
                    &session::on_write,
                    shared_from_this()));
            return;
        }

        write_in_progress_ = false;
    }

    void do_close(websocket::close_reason reason)
    {
        if (!ws_.is_open())
        {
            notifyCloseOnce();
            return;
        }

        ws_.async_close(
            reason,
            beast::bind_front_handler(
                &session::on_close,
                shared_from_this()));
    }

    void on_close(beast::error_code ec)
    {
        if (ec)
        {
            reportError(ec, "close");
        }

        notifyCloseOnce();
    }

    void notifyCloseOnce()
    {
        bool should_notify = false;
        {
            std::lock_guard<std::mutex> lock(message_mutex_);
            if (!close_notified_)
            {
                close_notified_ = true;
                should_notify = true;
            }
        }

        if (!should_notify)
        {
            return;
        }

        message_cv_.notify_all();
        owner_.notifyClose(shared_from_this());
    }

    void reportError(beast::error_code ec, std::string_view what)
    {
        owner_.notifyError(shared_from_this(), ec, what);
    }

    websocket::stream<beast::tcp_stream> ws_;
    beast::flat_buffer buffer_;
    std::deque<std::string> write_queue_;
    bool write_in_progress_{false};
    bool close_notified_{false};
    DebugServer& owner_;
    std::mutex message_mutex_;
    std::condition_variable message_cv_;
    std::deque<nlohmann::json> message_queue_;
};

//------------------------------------------------------------------------------

// Accepts incoming connections and launches the sessions
class listener : public std::enable_shared_from_this<listener>
{
    net::io_context& ioc_;
    tcp::acceptor acceptor_;
    DebugServer& owner_;

public:
    listener(
        net::io_context& ioc,
        tcp::endpoint endpoint,
        DebugServer& owner)
        : ioc_(ioc)
        , acceptor_(ioc)
        , owner_(owner)
    {
        beast::error_code ec;

        acceptor_.open(endpoint.protocol(), ec);
        if (ec)
        {
            owner_.notifyError(std::shared_ptr<session>{}, ec, "open");
            return;
        }

        acceptor_.set_option(net::socket_base::reuse_address(true), ec);
        if (ec)
        {
            owner_.notifyError(std::shared_ptr<session>{}, ec, "set_option");
            return;
        }

        acceptor_.bind(endpoint, ec);
        if (ec)
        {
            owner_.notifyError(std::shared_ptr<session>{}, ec, "bind");
            return;
        }

        acceptor_.listen(
            net::socket_base::max_listen_connections, ec);
        if (ec)
        {
            owner_.notifyError(std::shared_ptr<session>{}, ec, "listen");
            return;
        }
    }

    void run()
    {
        if (!acceptor_.is_open())
        {
            return;
        }

        do_accept();
    }

private:
    void do_accept()
    {
        if (!acceptor_.is_open())
        {
            return;
        }

        acceptor_.async_accept(
            net::make_strand(ioc_),
            beast::bind_front_handler(
                &listener::on_accept,
                shared_from_this()));
    }

    void on_accept(beast::error_code ec, tcp::socket socket)
    {
        if (ec)
        {
            owner_.notifyError(std::shared_ptr<session>{}, ec, "accept");
        }
        else
        {
            auto new_session = std::make_shared<session>(std::move(socket), owner_);
            new_session->run();
        }

        do_accept();
    }
};

//------------------------------------------------------------------------------

inline DebugServer::DebugServer(unsigned short port, int threads, std::string address)
    : address_(address.empty() ? "127.0.0.1" : std::move(address))
    , port_(port)
    , thread_count_(std::max(1, threads))
{
}

inline void DebugServer::setAddress(std::string address)
{
    address_ = address.empty() ? "127.0.0.1" : std::move(address);
}

inline void DebugServer::setPort(unsigned short port)
{
    port_ = port;
}

inline void DebugServer::setThreadCount(int threads)
{
    thread_count_ = std::max(1, threads);
}

inline void DebugServer::setOnOpen(OpenHandler handler)
{
    on_open_ = std::move(handler);
}

inline void DebugServer::setOnMessage(MessageHandler handler)
{
    on_message_ = std::move(handler);
}

inline void DebugServer::setOnClose(CloseHandler handler)
{
    on_close_ = std::move(handler);
}

inline void DebugServer::setOnError(ErrorHandler handler)
{
    on_error_ = std::move(handler);
}

inline int DebugServer::run()
{
    if (port_ == 0)
    {
        std::cerr << "DebugServer port not set" << std::endl;
        return EXIT_FAILURE;
    }

    beast::error_code ec;
    auto address = net::ip::make_address(address_, ec);
    if (ec)
    {
        notifyError(std::shared_ptr<session>{}, ec, "make_address");
        return EXIT_FAILURE;
    }

    net::io_context ioc{thread_count_};
    auto listener_instance = std::make_shared<listener>(ioc, tcp::endpoint{address, port_}, *this);
    listener_instance->run();

    std::vector<std::thread> thread_pool;
    if (thread_count_ > 1)
    {
        thread_pool.reserve(static_cast<std::size_t>(thread_count_ - 1));
        for (int i = 1; i < thread_count_; ++i)
        {
            thread_pool.emplace_back([&ioc]()
            {
                ioc.run();
            });
        }
    }

    ioc.run();

    for (auto& thread : thread_pool)
    {
        if (thread.joinable())
        {
            thread.join();
        }
    }

    return EXIT_SUCCESS;
}

inline DebugServer::SessionHandle DebugServer::makeHandle(const std::shared_ptr<session>& session_ptr) const
{
    if (!session_ptr)
    {
        return SessionHandle{};
    }

    return SessionHandle{session_ptr};
}

inline void DebugServer::notifyOpen(const std::shared_ptr<session>& session_ptr)
{
    if (!on_open_)
    {
        return;
    }

    on_open_(makeHandle(session_ptr));
}

inline void DebugServer::notifyMessage(const std::shared_ptr<session>& session_ptr, const nlohmann::json& message)
{
    if (!on_message_)
    {
        return;
    }

    on_message_(makeHandle(session_ptr), message);
}

inline void DebugServer::notifyClose(const std::shared_ptr<session>& session_ptr)
{
    if (!on_close_)
    {
        return;
    }

    on_close_(makeHandle(session_ptr));
}

inline void DebugServer::notifyError(const std::shared_ptr<session>& session_ptr, beast::error_code ec, std::string_view what)
{
    std::string label{what};
    fail(ec, label.c_str());

    if (!on_error_)
    {
        return;
    }

    on_error_(makeHandle(session_ptr), ec, what);
}

inline bool DebugServer::SessionHandle::isOpen() const
{
    auto locked = session_.lock();
    return locked && locked->isOpen();
}

inline void DebugServer::SessionHandle::send(nlohmann::json message) const
{
    if (auto locked = session_.lock())
    {
        locked->send(std::move(message));
    }
}

inline void DebugServer::SessionHandle::close(websocket::close_reason reason) const
{
    if (auto locked = session_.lock())
    {
        locked->close(std::move(reason));
    }
}

inline bool DebugServer::SessionHandle::waitForMessage(nlohmann::json& message) const
{
    return waitForMessage(message, std::chrono::milliseconds::max());
}

inline bool DebugServer::SessionHandle::waitForMessage(nlohmann::json& message, std::chrono::milliseconds timeout) const
{
    if (auto locked = session_.lock())
    {
        return locked->waitForMessage(message, timeout);
    }

    return false;
}

inline bool session::waitForMessage(nlohmann::json& message)
{
    return waitForMessage(message, std::chrono::milliseconds::max());
}

inline bool session::waitForMessage(nlohmann::json& message, std::chrono::milliseconds timeout)
{
    std::unique_lock<std::mutex> lock(message_mutex_);
    auto predicate = [this]()
    {
        return !message_queue_.empty() || close_notified_;
    };

    if (timeout == std::chrono::milliseconds::max())
    {
        message_cv_.wait(lock, predicate);
    }
    else if (!message_cv_.wait_for(lock, timeout, predicate))
    {
        return false;
    }

    if (message_queue_.empty())
    {
        return false;
    }

    message = std::move(message_queue_.front());
    message_queue_.pop_front();
    return true;
}