#include <iostream>
#include <string>
#include <curl/curl.h>
#include <jsoncpp/json/json.h>

// Geth node URL
std::string ethereum_node_url = "http://localhost:8545";

// Helper function to send JSON-RPC requests
std::string send_jsonrpc_request(const std::string& json_request) {
    CURL* curl = curl_easy_init();
    std::string result;

    if (curl) {
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

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "CURL Error: " << curl_easy_strerror(res) << std::endl;
        }

        curl_easy_cleanup(curl);
    }
    return result;
}

// Function to unlock an account
bool unlock_account(const std::string& account, const std::string& password) {
    std::string json_request = R"({"jsonrpc":"2.0","method":"personal_unlockAccount","params":[")" + account + R"(","" + password + R"(",60],"id":1})";
    std::string response = send_jsonrpc_request(json_request);

    Json::Value root;
    Json::CharReaderBuilder reader;
    std::string errs;

    std::istringstream s(response);
    if (Json::parseFromStream(reader, s, &root, &errs)) {
        return root["result"].asBool();
    } else {
        std::cerr << "Failed to parse unlock response: " << errs << std::endl;
    }
    return false;
}

// Function to send ETH
std::string send_eth(const std::string& from, const std::string& to, const std::string& value_in_wei) {
    std::string json_request = R"({"jsonrpc":"2.0","method":"eth_sendTransaction","params":[{"from":")" + from + R"(","to":")" + to + R"(","value":")" + value_in_wei + R"("}],"id":1})";
    return send_jsonrpc_request(json_request);
}

int main() {
    std::string from_account, to_account, password, value_in_wei;

    // Get user input
    std::cout << "Enter the sender account address: ";
    std::cin >> from_account;

    std::cout << "Enter the sender account password: ";
    std::cin >> password;

    std::cout << "Enter the recipient account address: ";
    std::cin >> to_account;

    std::cout << "Enter the value to send in wei (1 ETH = 10^18 wei): ";
    std::cin >> value_in_wei;

    // Unlock the sender account
    if (!unlock_account(from_account, password)) {
        std::cerr << "Failed to unlock account. Ensure the account is imported and the password is correct." << std::endl;
        return 1;
    }
    std::cout << "Account unlocked successfully." << std::endl;

    // Send the transaction
    std::string response = send_eth(from_account, to_account, value_in_wei);

    // Parse and display the transaction hash
    Json::Value root;
    Json::CharReaderBuilder reader;
    std::string errs;

    std::istringstream s(response);
    if (Json::parseFromStream(reader, s, &root, &errs)) {
        if (root.isMember("result")) {
            std::cout << "Transaction sent successfully!" << std::endl;
            std::cout << "Transaction Hash: " << root["result"].asString() << std::endl;
        } else {
            std::cerr << "Transaction failed: " << root.toStyledString() << std::endl;
        }
    } else {
        std::cerr << "Failed to parse transaction response: " << errs << std::endl;
    }

    return 0;
}
