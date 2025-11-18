#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <nlohmann/json.hpp>
#include <ProgramStructure.hpp>
#include <Builder.hpp>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

class session;
class DebugServer: std::enable_shared_from_this<DebugServer>
{
public:
    DebugServer(unsigned short port);
    ~DebugServer();

    void setPort(unsigned short port) { port_ = port; }
    void start();
    void stop();
    bool isRunning();
    void listenerLoop();
    void setOnNewSessionCallback(std::function<void(std::shared_ptr<session>)> callback)
    {
        onNewSession_ = callback;
    }

    void join();

    friend class session;
private:
    unsigned short port_;
    std::thread listener_thread_;
    std::atomic<bool> running_;
    std::vector<std::shared_ptr<session>> sessions_;
    std::function<void(std::shared_ptr<session>)> onNewSession_;
};

class session: public std::enable_shared_from_this<session>
{
public:
    session(tcp::socket socket, std::shared_ptr<DebugServer> server);
    void start();
    void stop();
    bool isRunning() const;
    bool sendMessage(nlohmann::json message);
    bool receiveMessage(nlohmann::json& message);
    void receiveLoop();
    void onMessage(nlohmann::json message);
    bool isPaused() const { return paused_; }
    void onTokenParsed(Token token);
    void beginParseOperation(const std::string& operation);
    void endParseOperation();
    void begin();
    void setBuilder(std::shared_ptr<Builder> b) { builder = b; }
    bool isConfigured() const { return configured_; }
private:
    std::atomic<bool> configured_{false};
    std::shared_ptr<Builder> builder;
    std::vector<Breakpoint> breakpoints_;
    std::atomic<bool> paused_=false;
    std::thread receive_thread_;
    websocket::stream<tcp::socket> ws_;
    std::shared_ptr<DebugServer> server_;
};