#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/system/error_code.hpp>
#include <cstdlib>
#include <iostream>
#include <string>
#include <utility>

#include <nlohmann/json.hpp>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

//------------------------------------------------------------------------------

inline void fail(beast::error_code ec, char const* what)
{
    std::cerr << what << ": " << ec.message() << "\n";
}

//------------------------------------------------------------------------------

class DebugClient
{
public:
    DebugClient() = default;

    DebugClient(std::string host, std::string port)
        : host_(std::move(host))
        , port_(std::move(port))
    {
    }

    ~DebugClient()
    {
        disconnect();
    }

    void setHost(std::string host)
    {
        host_ = std::move(host);
    }

    void setPort(std::string port)
    {
        port_ = std::move(port);
    }

    bool connect();
    void disconnect();

    bool send(const nlohmann::json& request);
    bool receive(nlohmann::json& response);
    nlohmann::json receive();
    bool isConnected() const;

private:
    bool ensureConnected();

    std::string host_{"127.0.0.1"};
    std::string port_{"80"};
    net::io_context ioc_{};
    websocket::stream<tcp::socket> ws_{ioc_};
    bool connected_{false};
};

inline bool DebugClient::ensureConnected()
{
    if (connected_)
    {
        return true;
    }

    return connect();
}

inline bool DebugClient::connect()
{
    if (connected_)
    {
        return true;
    }

    if (host_.empty() || port_.empty())
    {
        std::cerr << "DebugClient host or port not set" << std::endl;
        return false;
    }

    beast::error_code ec;
    tcp::resolver resolver{ioc_};
    auto results = resolver.resolve(host_, port_, ec);
    if (ec)
    {
        fail(ec, "resolve");
        return false;
    }

    auto endpoint = net::connect(ws_.next_layer(), results, ec);
    if (ec)
    {
        fail(ec, "connect");
        return false;
    }

    std::string host_header = host_;
    if (host_header.find(':') == std::string::npos)
    {
        host_header += ':' + std::to_string(endpoint.port());
    }

    ws_.set_option(websocket::stream_base::timeout::suggested(beast::role_type::client));

    ws_.handshake(host_header, "/", ec);
    if (ec)
    {
        fail(ec, "handshake");
        beast::error_code close_ec;
        ws_.next_layer().close(close_ec);
        if (close_ec)
        {
            fail(close_ec, "close");
        }
        return false;
    }

    connected_ = true;
    return true;
}

inline void DebugClient::disconnect()
{
    if (!connected_)
    {
        return;
    }

    beast::error_code ec;
    ws_.close(websocket::close_code::normal, ec);
    if (ec)
    {
        fail(ec, "close");
    }

    ws_ = websocket::stream<tcp::socket>{ioc_};
    connected_ = false;
}

inline bool DebugClient::send(const nlohmann::json& request)
{
    if (!ensureConnected())
    {
        return false;
    }

    const std::string payload = request.dump();
    ws_.text(true);

    beast::error_code ec;
    ws_.write(net::buffer(payload), ec);
    if (ec)
    {
        fail(ec, "write");
        return false;
    }

    return true;
}

inline bool DebugClient::receive(nlohmann::json& response)
{
    if (!ensureConnected())
    {
        return false;
    }

    beast::flat_buffer buffer;
    beast::error_code ec;
    ws_.read(buffer, ec);
    if (ec)
    {
        fail(ec, "read");
        return false;
    }

    std::string response_text = beast::buffers_to_string(buffer.cdata());
    buffer.consume(buffer.size());

    try
    {
        response = nlohmann::json::parse(response_text);
    }
    catch (const nlohmann::json::parse_error& ex)
    {
        auto parse_ec = boost::system::errc::make_error_code(boost::system::errc::invalid_argument);
        fail(parse_ec, ex.what());
        return false;
    }

    return true;
}

inline nlohmann::json DebugClient::receive()
{
    nlohmann::json response;
    if (!receive(response))
    {
        return nlohmann::json{};
    }
    return response;
}

inline bool DebugClient::isConnected() const
{
    return connected_;
}
