// std::string uniswap_univeral_address = "0x3fC91A3afd70395Cd496C647d5a6CC9D4B2b7FAD";
#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>
#include <iostream>
#include <string>
#include <json/json.h>
#include <curl/curl.h>
#include <thread>
#include <chrono>
#include <memory>

#include <vector>

#include <ctype.h> // used to make FROM filtered address all lowercase
#include <stdio.h> // used to make FROM filtered address all lowercase

typedef websocketpp::client<websocketpp::config::asio_tls_client> client;

std::string ethereum_node_url; // Will be initialized in main()
std::string from_address_filter = ""; // Will be initialized in main()

// Helper function to make an HTTP POST request to get transaction details by hash
std::string get_transaction_details(const std::string& tx_hash) {
    CURL* curl = curl_easy_init();
    std::string result;

    if(curl) {
        std::string json_request = R"({"jsonrpc":"2.0","method":"eth_getTransactionByHash","params":[")" + tx_hash + R"("],"id":1})";

        curl_easy_setopt(curl, CURLOPT_URL, ethereum_node_url.c_str());
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
            std::stringstream ss; // used to convert string hex to integer
            
            std::string from = result["from"].asString();
            //std::cout << result["from"].asString() << std::endl;
            //std::cout << from_address_filter << std::endl << std::endl;
            if (from == from_address_filter)
            {
                std::string to = result["to"].asString();

                ss << std::hex << result["gas"].asString();
                long long gas;
                ss >> gas;
                ss.clear();

                ss << std::hex << result["gasPrice"].asString();
                long long gasPrice;
                ss >> gasPrice;
                ss.clear();

                ss << std::hex << result["value"].asString();
                long long value;
                ss >> value;

                std::cout << "--------------------------" << std::endl;
                std::cout << "From: " << from << "\nTo: " << to 
                << "\nValue: " << result["value"].asString() << " (" << value  << ") "<< "\nGas: " << result["gas"].asString() 
                << " (" << gas << ") "
                << "\nGas Price: " << result["gasPrice"].asString() << " (" << gasPrice << ") " << "\n" << std::endl;
            }
        } else {
            //std::cerr << "Transaction details not found." << std::endl; // good for debugging purposes
        }
    } else {
        //std::cerr << "Failed to parse JSON: " << errs << std::endl;
    }
}

// Function to initialize the WebSocket connection and handle reconnection
void init() {

    boost::asio::thread_pool thread_pool(15); // thread pool of size 15!

    client ws_client;
    ws_client.init_asio();

    ws_client.set_tls_init_handler([](websocketpp::connection_hdl) {
        auto ctx = std::make_shared<websocketpp::lib::asio::ssl::context>(websocketpp::lib::asio::ssl::context::tlsv12);
        ctx->set_options(websocketpp::lib::asio::ssl::context::default_workarounds |
                         websocketpp::lib::asio::ssl::context::no_sslv2 |
                         websocketpp::lib::asio::ssl::context::no_sslv3 |
                         websocketpp::lib::asio::ssl::context::single_dh_use);
        return ctx;
    });

    ws_client.set_open_handler([&ws_client](websocketpp::connection_hdl hdl) {
        std::string subscribe_message = R"({"jsonrpc":"2.0","id":1,"method":"eth_subscribe","params":["newPendingTransactions"]})";
        ws_client.send(hdl, subscribe_message, websocketpp::frame::opcode::text);
        std::cout << "Subscribed to pending transactions" << std::endl;
    });

    ws_client.set_message_handler([&ws_client, &thread_pool](websocketpp::connection_hdl, client::message_ptr msg) {

        boost::asio::post(thread_pool, [msg]() {
        Json::Value root;
        Json::CharReaderBuilder reader;
        std::string errs;
        std::istringstream s(msg->get_payload());

        if (Json::parseFromStream(reader, s, &root, &errs)) {
            std::string tx_hash = root["params"]["result"].asString();
            //std::cout << "Pending Transaction Hash: " << tx_hash << std::endl; //leaving here 4 debugging later

            // Retrieve and print transaction details
            std::string tx_details = get_transaction_details(tx_hash);
            print_transaction_details(tx_details);
        } else {
            //std::cerr << "Failed to parse pending transaction JSON: " << errs << std::endl;
        }
        });

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
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <Ethereum Node URL>" << " <To Address>" << std::endl;
        return 1;
    }
    
    ethereum_node_url = argv[1]; // Set the URL from the command line argument
    from_address_filter = argv[2];

    // convert address filter to a lowercase string
    std::string temp = "";
    for (char c : from_address_filter)
        temp += (char) tolower(c);
    from_address_filter = temp;


    init();
    return 0;
}

