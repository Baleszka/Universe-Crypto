#include <iostream>
#include <string>
#include <vector>
#include <iomanip>
#include <cstdlib>
#include <stdlib.h>
#include <sstream>

#include <curl/curl.h>

#include <openssl/sha.h>
#include <openssl/evp.h>

size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

std::string make_post_request(const std::string& url, const std::string& post_data) {
    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "POST failed: " << curl_easy_strerror(res) << std::endl;
            readBuffer = "Error: Network request failed.";
        }
        curl_easy_cleanup(curl);
    } else {
        readBuffer = "Error: Could not initialize cURL.";
    }
    return readBuffer;
}

std::string make_get_request(const std::string& url) {
    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "GET failed: " << curl_easy_strerror(res) << std::endl;
            readBuffer = "Error: Network request failed.";
        }
        curl_easy_cleanup(curl);
    } else {
        readBuffer = "Error: Could not initialize cURL.";
    }
    return readBuffer;
}

std::string sha256_hash(const std::string& input) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, input.c_str(), input.length());
    SHA256_Final(hash, &sha256);

    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}

std::string sha1_hash(const std::string& input) {
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA_CTX sha1;
    SHA1_Init(&sha1);
    SHA1_Update(&sha1, input.c_str(), input.length());
    SHA1_Final(hash, &sha1);

    std::stringstream ss;
    for (int i = 0; i < SHA_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}

void clear_screen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void menu() {
    std::cout << "\nYOUR CRYPTO MENU\n\n";
    std::cout << "1. Check balance\n\n";
    std::cout << "2. Send crypto\n\n";
    std::cout << "3. Start mining\n\n";
    std::cout << "4. Change password\n\n";
    std::cout << "5. Check wallet address\n\n";
}

int main() {
    curl_global_init(CURL_GLOBAL_DEFAULT);

    std::string opt;
    std::string username, password;
    std::string usrh, passwdh;

    std::cout << "Welcome to your wallet!\n";
    std::cout << "Would you like to (l)og in or create a (n)ew account: ";
    std::cin >> opt;
    if (opt.length() == 1) opt[0] = std::tolower(opt[0]);

    if (opt == "l") {
        std::cout << "Enter username: ";
        std::cin >> username;
        usrh = sha1_hash(username);
        std::cout << "Enter password: ";
        std::cin >> password;
        passwdh = sha256_hash(password);

        std::string post_data = "username=" + usrh + "&password=" + passwdh;
        std::string response_text = make_post_request("http:localhost:8000/login", post_data);
        clear_screen();
        std::cout << response_text << std::endl;
        if (response_text.find("Incorrect password") != std::string::npos ||
            response_text.find("Missing username or password") != std::string::npos ||
            response_text.find("Account not found") != std::string::npos) return 0;
    }
    else if (opt == "n") {
        std::cout << "Enter username: ";
        std::cin >> username;
        usrh = sha1_hash(username);
        std::cout << "Enter password: ";
        std::cin >> password;
        passwdh = sha256_hash(password);

        std::string post_data = "username=" + usrh + "&password=" + passwdh;
        std::string response_text = make_post_request("http:localhost:8000/create_account", post_data);
        clear_screen();
        std::cout << response_text << std::endl;
        if (response_text.find("success") == std::string::npos) return 0;
    }
    else {
        std::cout << "Incorrect option. Please enter either 'l' or 'n'!\n";
        return 0;
    }

    while (true) {
        clear_screen();
        menu();
        std::cout << "Choose (or 'q' to quit): ";
        std::cin >> opt;
        if (opt.length() == 1) opt[0] = std::tolower(opt[0]);

        if (opt == "q") {
            std::cout << "Goodbye!\n";
            break;
        }

        if (opt == "1") {
            std::string post_data = "username=" + usrh;
            std::string response_text = make_post_request("http:localhost:8000/check_balance", post_data);
            std::cout << response_text << std::endl;
        }
        else if (opt == "2") {
            std::string receiver_addr, amount;
            std::cout << "Enter the address of the receiver: ";
            std::cin >> receiver_addr;
            std::cout << "Enter how much you want to send: ";
            std::cin >> amount;

            std::string post_data = "sender=" + usrh + "&reciever=" + receiver_addr + "&amount=" + amount;
            std::string response_text = make_post_request("http:localhost:8000/send_crypto", post_data);
            std::cout << "\nResponse: " << response_text << std::endl;
        }
        else if (opt == "3") {
            system("./miner");
        }
        else if (opt == "4") {
            std::cout << "Lazy to do lol.\n";
        }
        else if (opt == "5") {
            std::string target_adress;
            std::cout << "Enter the user to get the adress of: ";
            std::cin >> target_adress;

            std::string url = "http:localhost:8000/get_adress?adress=" + target_adress;
            std::string response_text = make_get_request(url);
            std::cout << "\n" << target_adress << "'s adress is " << response_text << std::endl;
        }
        else {
            std::cout << "Invalid option.\n";
        }

        std::cout << "Press Enter to continue...";
        std::cin.ignore();
        std::cin.get();
    }

    curl_global_cleanup();
    return 0;
}
