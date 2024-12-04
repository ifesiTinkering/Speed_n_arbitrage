 #include <iostream>
#include <string>
#include <curl/curl.h>
#include <jsoncpp/json/json.h>
#include <thread>
#include <chrono>

// Geth node URL
std::string ethereum_node_url = "http://localhost:8545";

// Uniswap Universal Router address
std::string uniswap_router_address = "0x3fc91a3afd70395cd496c647d5a6cc9d4b2b7fad";

// Helper function to make an HTTP POST request to query txpool_content
std::string get_txpool_content() {
    CURL* curl = curl_easy_init();
    std::string result;

    if (curl) {
        std::string json_request = R"({"jsonrpc":"2.0","method":"txpool_content","params":[],"id":1})";

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

// Function to continuously check for transactions targeting the Uniswap Universal Router
bool find_and_print_uniswap_transaction() {
    std::string response = get_txpool_content();
    Json::Value root;
    Json::CharReaderBuilder reader;
    std::string errs;

    std::istringstream s(response);
    if (Json::parseFromStream(reader, s, &root, &errs)) {
        Json::Value pending = root["result"]["pending"];
        if (pending.isObject()) {
            // Iterate through all addresses in the pending transactions
            for (const auto& sender : pending.getMemberNames()) {
                const Json::Value& senderTxs = pending[sender];
                for (const auto& nonce : senderTxs.getMemberNames()) {
                    const Json::Value& tx = senderTxs[nonce];
                    if (tx.isMember("to") && tx["to"].asString() == uniswap_router_address) {
                        std::cout << "Transaction found to Uniswap Universal Router!" << std::endl;
                        std::cout << "Hash: " << tx["hash"].asString() << std::endl;
                        return true; // Exit once a transaction is found
                    }
                }
            }
        } else {
            std::cout << "No pending transactions found in txpool." << std::endl;
        }
    } else {
        std::cerr << "Failed to parse JSON: " << errs << std::endl;
    }
    return false; // No matching transaction found
}

int main() {
    std::cout << "Monitoring mempool for transactions to Uniswap Universal Router..." << std::endl;

    while (true) {
        if (find_and_print_uniswap_transaction()) {
            break; // Stop looking once a transaction is found
        }
        std::cout << "No transaction found, retrying..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(2)); // Poll every 2 seconds
    }

    std::cout << "Transaction found, exiting." << std::endl;
    return 0;
}