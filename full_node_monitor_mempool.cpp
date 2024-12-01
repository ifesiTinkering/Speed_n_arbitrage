#include <iostream>
#include <string>
#include <curl/curl.h>
#include <jsoncpp/json/json.h>
#include <thread>
#include <chrono>

// Geth node URL
std::string ethereum_node_url = "http://localhost:8545";

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

// Function to filter transactions from a specific address
bool find_pending_transaction(const std::string& address) {
    std::string response = get_txpool_content();
    Json::Value root;
    Json::CharReaderBuilder reader;
    std::string errs;

    std::istringstream s(response);
    if (Json::parseFromStream(reader, s, &root, &errs)) {
        Json::Value pending = root["result"]["pending"];
        if (pending.isObject() && pending.isMember(address)) {
            // Iterate over pending transactions for the given address
            for (const auto& nonce : pending[address].getMemberNames()) {
                const Json::Value& tx = pending[address][nonce];
                std::cout << "Transaction found!" << std::endl;
                std::cout << "Nonce: " << nonce << std::endl;
                std::cout << "To: " << tx["to"].asString() << std::endl;
                std::cout << "Value: " << tx["value"].asString() << std::endl;
                std::cout << "Gas: " << tx["gas"].asString() << std::endl;
                std::cout << "Gas Price: " << tx["gasPrice"].asString() << std::endl;
                std::cout << "Hash: " << tx["hash"].asString() << std::endl;
                std::cout << "--------------------------" << std::endl;
                return true; // Stop after finding the first transaction
            }
        }
    } else {
        std::cerr << "Failed to parse JSON: " << errs << std::endl;
    }
    return false; // No transaction found for the address
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <Wallet Address>" << std::endl;
        return 1;
    }

    std::string filter_address = argv[1];
    std::cout << "Monitoring mempool for transactions from: " << filter_address << std::endl;

    while (true) {
        if (find_pending_transaction(filter_address)) {
            break; // Exit once a transaction is found
        }
        std::cout << "Waiting for a transaction..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(5)); // Poll every 5 seconds
    }

    return 0;
}
