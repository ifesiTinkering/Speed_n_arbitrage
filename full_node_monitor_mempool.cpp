#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>
#include <jsoncpp/json/json.h>

#include <iostream>
#include <string>
#include <curl/curl.h>
#include <thread>
#include <chrono>
std::string filter_address; // Wallet address to filter transactions
typedef websocketpp::client<websocketpp::config::asio_client> client; // Non-TLS WebSocket client

std::string ethereum_node_url = "ws://localhost:8546"; // Local Geth WebSocket URL

// Helper function to make an HTTP POST request to get transaction details by hash
std::string get_transaction_details(const std::string& tx_hash) {
    CURL* curl = curl_easy_init();
    std::string result;

    if (curl) {
        std::string json_request = R"({"jsonrpc":"2.0","method":"eth_getTransactionByHash","params":[")" + tx_hash + R"("],"id":1})";

        // Use the HTTP endpoint of Geth for transaction details
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

// Function to parse and print transaction details from JSON response
void print_transaction_details(const std::string& json_response) {
    Json::Value root;
    Json::CharReaderBuilder reader;
    std::string errs;

    std::istringstream s(json_response);
    if (Json::parseFromStream(reader, s, &root, &errs)) {
        auto result = root["result"];
        if (!result.isNull()) {
            std::string from = result["from"].asString();
            
            

            std::string to = result["to"].isNull() ? "Contract Creation" : result["to"].asString();
            std::string value = result["value"].asString();
            std::string gas = result["gas"].asString();
            std::string gasPrice = result["gasPrice"].asString();

            std::cout << "--------------------------" << std::endl;
            std::cout << "From: " << from << "\nTo: " << to 
                      << "\nValue: " << value << "\nGas: " << gas 
                      << "\nGas Price: " << gasPrice << "\n" << std::endl;



        } else {
            std::cerr << "Transaction details not found." << std::endl;
        }
    } else {
        std::cerr << "Failed to parse JSON: " << errs << std::endl;
    }
}

// Function to initialize the WebSocket connection and handle reconnection
void init() {
    client ws_client;
    ws_client.init_asio();

    ws_client.set_open_handler([&ws_client](websocketpp::connection_hdl hdl) {
        std::string subscribe_message = R"({"jsonrpc":"2.0","id":1,"method":"eth_subscribe","params":["newPendingTransactions"]})";
        ws_client.send(hdl, subscribe_message, websocketpp::frame::opcode::text);
        std::cout << "Subscribed to pending transactions" << std::endl;
    });

    ws_client.set_message_handler([&ws_client](websocketpp::connection_hdl, client::message_ptr msg) {
        Json::Value root;
        Json::CharReaderBuilder reader;
        std::string errs;
        std::istringstream s(msg->get_payload());

        if (Json::parseFromStream(reader, s, &root, &errs)) {
            std::string tx_hash = root["params"]["result"].asString();
            std::cout << "Pending Transaction Hash: " << tx_hash << std::endl;

            // Retrieve and print transaction details
            std::string tx_details = get_transaction_details(tx_hash);
            print_transaction_details(tx_details);
        } else {
            std::cerr << "Failed to parse pending transaction JSON: " << errs << std::endl;
        }
    });

    // Error and close handlers for reconnection
    ws_client.set_fail_handler([&ws_client](websocketpp::connection_hdl) {
        std::cerr << "Unable to connect, retrying in 3 seconds..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(3));
        init();
    });

    ws_client.set_close_handler([&ws_client](websocketpp::connection_hdl) {
        std::cerr << "Connection closed! Attempting to reconnect in 3 seconds..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(3));
        init();
    });

    websocketpp::lib::error_code ec;
    client::connection_ptr con = ws_client.get_connection(ethereum_node_url, ec);

    if (ec) {
        std::cerr << "Could not create connection because: " << ec.message() << std::endl;
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

    // Get the wallet address from command-line arguments
    filter_address = argv[1];
    std::cout << "Filtering transactions from wallet: " << filter_address << std::endl;

    init();
    return 0;
}
