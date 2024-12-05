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

#include<chrono>

typedef websocketpp::client<websocketpp::config::asio_tls_client> client;

const std::string UNISWAP_MEMPOOL_ADDRESS = "0x3fc91a3afd70395cd496c647d5a6cc9d4b2b7fad";

// Helper function to make an HTTP POST request to get transaction details by hash
std::string get_transaction_details(const std::string& tx_hash, std::string &endpoint_url) {
    CURL* curl = curl_easy_init();
    std::string result;

    if(curl) {
        std::string json_request = R"({"jsonrpc":"2.0","method":"eth_getTransactionByHash","params":[")" + tx_hash + R"("],"id":1})";

        curl_easy_setopt(curl, CURLOPT_URL, endpoint_url.c_str());
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

// Function to parse transaction details from JSON response to determine if it comes from UNISWAP address
bool is_transaction_to_uniswap(const std::string& json_response, int &transactions_printed) {
    Json::Value root;
    Json::CharReaderBuilder reader;
    std::string errs;

    std::istringstream s(json_response);
    if (Json::parseFromStream(reader, s, &root, &errs)) {
        auto result = root["result"];
        if (!result.isNull()) {
            std::stringstream ss; // used to convert string hex to integer
            std::string to = result["to"].asString();
            if (to == UNISWAP_MEMPOOL_ADDRESS)
            {
                transactions_printed++;
                return true;
            }
        }
    }
    return false;
}

// Function to initialize the WebSocket connection and handle reconnection
void parallel_init(int count, std::vector<std::string> &transaction_hashes, int &transactions_printed, std::string &endpoint_url) {

    boost::asio::thread_pool thread_pool(15); // thread pool of size 15!

    client ws_client;
    ws_client.init_asio();
    ws_client.clear_access_channels(websocketpp::log::alevel::all);


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

    ws_client.set_message_handler([&ws_client, &thread_pool, &count, &transaction_hashes, &transactions_printed, &endpoint_url](websocketpp::connection_hdl, client::message_ptr msg) {
        if (transactions_printed < count)
        {
            boost::asio::post(thread_pool, [msg, &count, &transaction_hashes, &transactions_printed, &endpoint_url]() {
                Json::Value root;
                Json::CharReaderBuilder reader;
                std::string errs;
                std::istringstream s(msg->get_payload());

                if (Json::parseFromStream(reader, s, &root, &errs)) {
                    std::string tx_hash = root["params"]["result"].asString();
                    //std::cout << "Pending Transaction Hash: " << tx_hash << std::endl; //leaving here 4 debugging later
                    // Retrieve and print transaction details
                    std::string tx_details = get_transaction_details(tx_hash, endpoint_url);
                    if (is_transaction_to_uniswap(tx_details, transactions_printed) & (transaction_hashes.size() <= count) )
                    {
                        transaction_hashes.push_back(tx_hash);
                        std::cout << tx_hash << " (" << transactions_printed << ") " << std::endl;
                    }

                } else {
                    //std::cerr << "Failed to parse pending transaction JSON: " << errs << std::endl;
                }
            });
        }
        else {
            ws_client.stop();
        }

    });

    // Error and close handlers for reconnection
    ws_client.set_fail_handler([&ws_client, &count, &transaction_hashes, &transactions_printed, &endpoint_url](websocketpp::connection_hdl) {
        std::cerr << "Unable to connect, retrying in 100 ms..." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        parallel_init(count, transaction_hashes, transactions_printed, endpoint_url);
    });

    ws_client.set_close_handler([&ws_client, &count, &transaction_hashes, &transactions_printed, &endpoint_url](websocketpp::connection_hdl) {
        std::cerr << "Connection closed! Attempting to reconnect in 100 ms..." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        parallel_init(count, transaction_hashes, transactions_printed, endpoint_url);
    });

    websocketpp::lib::error_code ec;
    client::connection_ptr con = ws_client.get_connection(endpoint_url, ec);

    if (ec) {
        std::cerr << "Could not create connection because: " << ec.message() << std::endl;
        return;
    }

    ws_client.connect(con);
    ws_client.run();
}


void serial_init(int count, std::vector<std::string> &transaction_hashes, int &transactions_printed, std::string &endpoint_url) {
    client ws_client;
    ws_client.init_asio();
    ws_client.clear_access_channels(websocketpp::log::alevel::all);

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

    ws_client.set_message_handler([&ws_client, &count, &transaction_hashes, &transactions_printed, &endpoint_url](websocketpp::connection_hdl, client::message_ptr msg) {
        if (transactions_printed < count)
        {
            Json::Value root;
            Json::CharReaderBuilder reader;
            std::string errs;
            std::istringstream s(msg->get_payload());

            if (Json::parseFromStream(reader, s, &root, &errs)) {
                std::string tx_hash = root["params"]["result"].asString();

                // Retrieve and print transaction details
                std::string tx_details = get_transaction_details(tx_hash, endpoint_url);
                if (is_transaction_to_uniswap(tx_details, transactions_printed) & (transaction_hashes.size() <= count) )
                {
                    std::cout << tx_hash << " (" << transactions_printed << ") " << std::endl;
                    transaction_hashes.push_back(tx_hash);
                }

            }
        }
        else {
            ws_client.stop();
        }

    });

    // Error and close handlers for reconnection
    ws_client.set_fail_handler([&ws_client, &count, &transaction_hashes, &transactions_printed, &endpoint_url](websocketpp::connection_hdl) {
        std::cerr << "Unable to connect, retrying in 100 ms..." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        serial_init(count, transaction_hashes, transactions_printed, endpoint_url);
    });

    ws_client.set_close_handler([&ws_client, &count, &transaction_hashes, &transactions_printed, &endpoint_url](websocketpp::connection_hdl) {
        std::cerr << "Connection closed! Attempting to reconnect in 100 ms..." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        serial_init(count, transaction_hashes, transactions_printed, endpoint_url);
    });

    websocketpp::lib::error_code ec;
    client::connection_ptr con = ws_client.get_connection(endpoint_url, ec);

    if (ec) {
        std::cerr << "Could not create connection because: " << ec.message() << std::endl;
        return;
    }

    ws_client.connect(con);
    ws_client.run();
}

long long serial_mempool_monitor(int count, std::string endpoint_url) // return how long it took to collect
{
    std::vector<std::string> transaction_hashes;
    int hashes_found = 0;
    auto t0 = std::chrono::system_clock::now();
    serial_init(count, transaction_hashes, hashes_found, endpoint_url);
    auto t1 = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(t1-t0).count();
}


long long parallel_mempool_monitor(int count, std::string endpoint_url)
{
    std::vector<std::string> transaction_hashes;
    int hashes_found = 0;
    auto t0 = std::chrono::system_clock::now();
    parallel_init(count, transaction_hashes, hashes_found, endpoint_url);
    auto t1 = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(t1-t0).count();
}

int main(int argc, char* argv[]) {
    if (argc != 4 || (argv[3] != std::string("p") && argv[3] != std::string("s"))) {
        std::cerr << "Usage: " << argv[0] << " <Ethereum Node URL>" << " <count>" << " <p/s>" << std::endl;
        return 1;
    }
    
    int transaction_count = std::stoi(argv[2]); 

    std::string endpoint_url = argv[1];
    std::string option = argv[3];

    if (option == "p")
    {
        long long time_taken = parallel_mempool_monitor(transaction_count, endpoint_url);
        std::cout << "PARALLEL TIME: " << time_taken << std::endl;
    }
    if (option == "s")
    {
        long long time_taken = serial_mempool_monitor(transaction_count, endpoint_url);
        std::cout << "SERIAL TIME: " << time_taken << std::endl;
    }

    return 0;
}

