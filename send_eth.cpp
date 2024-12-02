#include <iostream>
#include <string>
#include <sstream>
#include <cstdlib>
#include <cstdio>
#include <json/json.h>

// Helper function to call Python script
std::string call_python_script(const std::string& maxFeePerGas, const std::string& maxPriorityFeePerGas, const std::string& gas) {
    std::string command = "python3 TransactionManager.py " + maxFeePerGas + " " + maxPriorityFeePerGas + " " + gas;
    std::string result;
    char buffer[128];

    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        std::cerr << "Failed to execute Python script." << std::endl;
        return "";
    }

    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }
    pclose(pipe);

    return result;
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <maxFeePerGas (Gwei)> <maxPriorityFeePerGas (Gwei)> <gas>" << std::endl;
        return 1;
    }

    std::string maxFeePerGas = argv[1];
    std::string maxPriorityFeePerGas = argv[2];
    std::string gas = argv[3];

    // Call the Python script
    std::string response = call_python_script(maxFeePerGas, maxPriorityFeePerGas, gas);

    // Parse the JSON response
    Json::Value root;
    Json::CharReaderBuilder reader;
    std::string errs;
    std::istringstream s(response);

    if (!Json::parseFromStream(reader, s, &root, &errs)) {
        std::cerr << "Failed to parse Python script response: " << errs << std::endl;
        std::cerr << "Raw Response: " << response << std::endl;
        return 1;
    }

    // Process the JSON response
    if (root["status"].asString() == "success") {
        std::cout << "Transaction executed successfully!" << std::endl;
        std::cout << "Transaction Hash: " << root["tx_hash"].asString() << std::endl;
    } else {
        std::cerr << "Error: " << root["message"].asString() << std::endl;
    }

    return 0;
}
