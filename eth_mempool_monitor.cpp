#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>

#include <iostream>
#include <string>
#include <memory>

typedef websocketpp::client<websocketpp::config::asio_tls_client> client;

std::string create_subscription_message() {
    return R"({"jsonrpc":"2.0","id":1,"method":"eth_subscribe","params":["newPendingTransactions"]})";
}

void on_message(websocketpp::connection_hdl, client::message_ptr msg) {
    std::cout << "Pending Transaction Message: " << msg->get_payload() << std::endl;
}

std::shared_ptr<websocketpp::lib::asio::ssl::context> on_tls_init(websocketpp::connection_hdl) {
    auto ctx = std::make_shared<websocketpp::lib::asio::ssl::context>(websocketpp::lib::asio::ssl::context::tlsv12);
    ctx->set_options(websocketpp::lib::asio::ssl::context::default_workarounds |
                     websocketpp::lib::asio::ssl::context::no_sslv2 |
                     websocketpp::lib::asio::ssl::context::no_sslv3 |
                     websocketpp::lib::asio::ssl::context::single_dh_use);
    return ctx;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <WebSocket URL>" << std::endl;
        return 1;
    }

    std::string websocket_url = argv[1];
    client ws_client;

    try {
        // Initialize ASIO
        ws_client.init_asio();

        // Set the TLS initialization handler
        ws_client.set_tls_init_handler(on_tls_init);

        // Set message handler
        ws_client.set_message_handler(&on_message);

        // Create a connection
        websocketpp::lib::error_code ec;
        client::connection_ptr con = ws_client.get_connection(websocket_url, ec);
        if (ec) {
            std::cout << "Could not create connection because: " << ec.message() << std::endl;
            return -1;
        }

        // Connect to the server
        ws_client.connect(con);

        // Send subscription message when connection is open
        con->set_open_handler([&ws_client, con](websocketpp::connection_hdl) {
            std::string subscription_message = create_subscription_message();
            ws_client.send(con, subscription_message, websocketpp::frame::opcode::text);
        });

        // Start the ASIO IO loop
        ws_client.run();
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}
