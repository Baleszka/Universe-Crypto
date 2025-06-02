#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <random>
#include <sstream>
#include <curl/curl.h>
#include <openssl/sha.h>
#include <iomanip>
#include <chrono>

std::atomic<bool> found(false);
std::string found_string;
std::atomic<bool> keep_mining(true);

const std::vector<char> charset1 = {'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z','1','2','3','4','5','6','7','8','9','0'};
const std::vector<char> charset2 = charset1;
const std::vector<char> charset3 = [](){
    std::vector<char> cs;
    for(char c = 'a'; c <= 'z'; ++c) cs.push_back(c);
    for(char c = 'A'; c <= 'Z'; ++c) cs.push_back(c);
    for(char c = '1'; c <= '9'; ++c) cs.push_back(c);
    cs.push_back('0');
    return cs;
}();

std::string sha256(const std::string& str) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((const unsigned char*)str.c_str(), str.size(), hash);
    std::stringstream ss;
    for (unsigned char byte : hash) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)byte;
    }
    return ss.str();
}

size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
    size_t total = size * nmemb;
    output->append((char*)contents, total);
    return total;
}

std::string fetch_target_hash(int difficulty) {
    CURL* curl = curl_easy_init();
    std::string response;
    if (curl) {
        std::string url = "http:localhost:8000/get_hash_" + std::to_string(difficulty);
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }
    return response;
}

bool post_result(const std::string& cracked, const std::string& hash, const std::string& wallet, int difficulty) {
    CURL* curl = curl_easy_init();
    std::string response;
    bool success = false;
    
    if (curl) {
        std::string postFields = "hash_string=" + cracked + "&hash=" + hash + "&wallet=" + wallet;
        std::string url = "http:localhost:8000/check_hash" + std::to_string(difficulty);

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postFields.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        CURLcode res = curl_easy_perform(curl);
        if (res == CURLE_OK) {
            std::cout << "Server response: " << response << std::endl;
            success = response.find("Mining successful") != std::string::npos;
        } else {
            std::cerr << "Failed to submit: " << curl_easy_strerror(res) << std::endl;
        }

        curl_easy_cleanup(curl);
    }
    return success;
}

void mine_thread(const std::string& target_hash, int difficulty, const std::vector<char>& charset, int length) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(0, charset.size() - 1);

    while (!found && keep_mining) {
        std::string attempt;
        for (int i = 0; i < length; ++i)
            attempt += charset[dist(gen)];

        std::string hashed = sha256(attempt);

        if (hashed == target_hash) {
            found = true;
            found_string = attempt;
            break;
        }
    }
}

int main() {
    std::string wallet;
    int difficulty;

    std::cout << "Auto-Mining Crypto Miner v1.0" << std::endl;
    std::cout << "=============================" << std::endl;
    
    std::cout << "Enter wallet address: ";
    std::cin >> wallet;

    std::cout << "Enter mining difficulty (1, 2, 3): ";
    std::cin >> difficulty;

    const std::vector<char>* charset;
    int str_length;

    switch (difficulty) {
        case 1: charset = &charset1; str_length = 4; break;
        case 2: charset = &charset2; str_length = 5; break;
        case 3: charset = &charset3; str_length = 5; break;
        default:
            std::cerr << "Invalid difficulty. Choose 1, 2, or 3." << std::endl;
            return 1;
    }

    std::cout << std::endl;
    std::cout << "Starting auto-mining... Press Ctrl+C to stop." << std::endl;
    std::cout << "Reward for difficulty " << difficulty << ": ";
    switch(difficulty) {
        case 1: std::cout << "0.03 coins per hash"; break;
        case 2: std::cout << "0.2 coins per hash"; break;
        case 3: std::cout << "0.5 coins per hash"; break;
    }
    std::cout << std::endl;
    std::cout << "Using " << std::thread::hardware_concurrency() << " CPU threads" << std::endl;
    std::cout << "========================================" << std::endl;

    int hashes_solved = 0;
    int num_threads = std::thread::hardware_concurrency();

    while (keep_mining) {
        found = false;
        found_string = "";

        std::string target_hash = fetch_target_hash(difficulty);
        if (target_hash.empty() || target_hash.find("Error") != std::string::npos) {
            std::cerr << "Failed to fetch hash from server. Retrying in 5 seconds..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(5));
            continue;
        }

        std::cout << "Mining hash #" << (hashes_solved + 1) << ": " << target_hash.substr(0, 16) << "..." << std::endl;
        
        auto start_time = std::chrono::high_resolution_clock::now();

        std::vector<std::thread> threads;
        for (int i = 0; i < num_threads; ++i) {
            threads.emplace_back(mine_thread, target_hash, difficulty, std::ref(*charset), str_length);
        }

        for (auto& t : threads) {
            t.join();
        }

        if (found) {
            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time);
            
            std::cout << "✓ Found solution: " << found_string << " (took " << duration.count() << "s)" << std::endl;
            
            if (post_result(found_string, target_hash, wallet, difficulty)) {
                hashes_solved++;
                std::cout << "✓ Successfully mined " << hashes_solved << " hash(es) total!" << std::endl;
            } else {
                std::cout << "✗ Failed to submit solution to server." << std::endl;
            }
        } else {
            std::cout << "Mining stopped by user." << std::endl;
            break;
        }

        std::cout << "----------------------------------------" << std::endl;
        
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    std::cout << std::endl;
    std::cout << "Mining session ended." << std::endl;
    std::cout << "Total hashes successfully solved: " << hashes_solved << std::endl;
    
    return 0;
}