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

    friend class session;
private:
    unsigned short port_;
    std::thread listener_thread_;
    std::atomic<bool> running_;
    std::vector<std::shared_ptr<session>> sessions_;
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
    void reveiveLoop();
    void onMessage(nlohmann::json message);
    bool isPaused() const { return paused_; }
    void onTokenParsed(Token token);
    void beginParseOperation(const std::string& operation);
    void endParseOperation();
private:
    bool paused_=false;
    std::filesystem::path schemaDirectory;
	std::filesystem::path schemaFile;
	std::filesystem::path outputDirectory;
	std::filesystem::path additionalGeneratorsDirectory;
	bool EnableExponentialOperations = false;
	bool recursive = false;
    std::vector<std::shared_ptr<Generator>> dynamicGenerators;
	std::vector<std::string> dynamicGeneratorNames;
    ProgramStructure program_structure_;
    std::thread receive_thread_;
    websocket::stream<tcp::socket> ws_;
    std::shared_ptr<DebugServer> server_;
};