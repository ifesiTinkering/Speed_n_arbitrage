#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>
#include <jsoncpp/json/json.h>
#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <iostream>
#include <curl/curl.h>



typedef websocketpp::client<websocketpp::config::asio_client> client;

std::string ethereum_node_url = "ws://localhost:8546"; // Geth WebSocket URL
std::string filter_address; // Address to monitor

// Helper function to fetch transaction details
std::string get_transaction_details(const std::string& tx_hash) {
    CURL* curl = curl_easy_init();
    std::string result;

    if (curl) {
        std::string json_request = R"({"jsonrpc":"2.0","method":"eth_getTransactionByHash","params":[")" + tx_hash + R"("],"id":1})";

        curl_easy_setopt(curl, CURLOPT_URL, "http://localhost:8545");
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_request.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, +[](void* data, size_t size, size_t nmemb, std::string* writerData) -> size_t {
            if (writerData == nullptr) return 0;
            writerData->append((char*)data, size * nmemb);
            return size * nmemb;
        });
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, curl_slist_append(nullptr, "Content-Type: application/json"));

        curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }
    return result;
}

// Print transaction details
void print_transaction_details(const std::string& tx_details) {
    Json::Value root;
    Json::CharReaderBuilder reader;
    std::string errs;

    std::istringstream s(tx_details);
    if (Json::parseFromStream(reader, s, &root, &errs)) {
        Json::Value result = root["result"];
        if (!result.isNull() && result["from"].asString() == filter_address) {
            std::cout << "Transaction from: " << result["from"].asString() << "\n";
            std::cout << "To: " << result["to"].asString() << "\n";
            std::cout << "Value: " << result["value"].asString() << "\n";
            std::cout << "Gas: " << result["gas"].asString() << "\n";
            std::cout << "Gas Price: " << result["gasPrice"].asString() << "\n";
            std::cout << "Hash: " << result["hash"].asString() << "\n";
            std::cout << "--------------------------" << std::endl;
        }
    } else {
        std::cerr << "Failed to parse transaction details: " << errs << std::endl;
    }
}

void monitor_pending_transactions() {
    client ws_client;
    ws_client.init_asio();

    ws_client.set_open_handler([&ws_client](websocketpp::connection_hdl hdl) {
        std::string subscribe_message = R"({"jsonrpc":"2.0","id":1,"method":"eth_subscribe","params":["newPendingTransactions"]})";
        ws_client.send(hdl, subscribe_message, websocketpp::frame::opcode::text);
        std::cout << "Subscribed to pending transactions." << std::endl;
    });

    ws_client.set_message_handler([&ws_client](websocketpp::connection_hdl, client::message_ptr msg) {
        Json::Value root;
        Json::CharReaderBuilder reader;
        std::string errs;

        std::istringstream s(msg->get_payload());
        if (Json::parseFromStream(reader, s, &root, &errs)) {
            std::string tx_hash = root["params"]["result"].asString();
            std::string tx_details = get_transaction_details(tx_hash);
            print_transaction_details(tx_details);
        } else {
            std::cerr << "Failed to parse WebSocket message: " << errs << std::endl;
        }
    });

    ws_client.set_fail_handler([&ws_client](websocketpp::connection_hdl) {
        std::cerr << "Connection failed. Retrying..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(3));
        monitor_pending_transactions(); // Retry
    });

    ws_client.set_close_handler([&ws_client](websocketpp::connection_hdl) {
        std::cerr << "Connection closed. Reconnecting..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(3));
        monitor_pending_transactions(); // Reconnect
    });

    websocketpp::lib::error_code ec;
    client::connection_ptr con = ws_client.get_connection(ethereum_node_url, ec);

    if (ec) {
        std::cerr << "Could not create connection: " << ec.message() << std::endl;
        return;
    }

    ws_client.connect(con);
    ws_client.run();
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <Wallet Address>" << std::endl;
        return 1;
    }

    filter_address = argv[1];
    std::cout << "Monitoring mempool for transactions from: " << filter_address << std::endl;

    monitor_pending_transactions();
    return 0;
}
