#include <iostream>
#include <string>
#include <sstream>
#include <curl/curl.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include "json.hpp" 
//g++ -o main main.cpp -I/opt/homebrew/opt/openssl@3/include -L/opt/homebrew/opt/openssl@3/lib -lssl -lcrypto -lcurl -std=c++11


using namespace std;
using json_lib = nlohmann::json;

const string CLIENT_KEY = "MAcxX_AD";
const string CLIENT_SECRET = "NbpI9aZrj4T8vFUdQ_7F_lOWKQRKRpwRSAHVaRx7o3I";
const string BASE_URL = "https://test.deribit.com/api/v2";

class TradingClient {
private:
//callback function used to handle incoming data when making HTTP requests
    static size_t process_response(void* buffer, size_t size, size_t nmemb, void* userdata) {
        ((string*)userdata)->append((char*)buffer, size * nmemb);
        return size * nmemb;
    }

    // GET Requests
    json_lib execute_get(const string& endpoint, const string& query = "", const bool& isAuth = false) {
    CURL* curl_session;
    CURLcode curl_result;
    string response_data;
    string auth_token;
    
    // Get the authentication token only when required (if isAuth is true)
    if (isAuth) {
        auth_token = get_authentication_token();
        if (auth_token.empty()) {
            cerr << "Failed to get authentication token." << endl;
            return nullptr;
        }
    }

    curl_session = curl_easy_init();
    if (curl_session) {
        string url = BASE_URL + "/" + endpoint;
        if (!query.empty()) {
            url += "?" + query;
        }
        curl_easy_setopt(curl_session, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl_session, CURLOPT_HTTPGET, 1L);

        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");

        if (isAuth && !auth_token.empty()) {
            string auth_header = "Authorization: Bearer " + auth_token;
            headers = curl_slist_append(headers, auth_header.c_str());
        }

        curl_easy_setopt(curl_session, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl_session, CURLOPT_WRITEFUNCTION, process_response);
        curl_easy_setopt(curl_session, CURLOPT_WRITEDATA, &response_data);

        curl_result = curl_easy_perform(curl_session);
        if (curl_result != CURLE_OK) {
            cerr << "cURL GET error: " << curl_easy_strerror(curl_result) << endl;
        }

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl_session);
    }

    try {
        return json_lib::parse(response_data);
    } catch (json_lib::parse_error& e) {
        cerr << "Error parsing GET response: " << e.what() << endl;
        return nullptr;
    }
}

public: 
    string get_authentication_token() {
        const string endpoint = "public/auth";
        const string query = "client_id=" + CLIENT_KEY +
                             "&client_secret=" + CLIENT_SECRET +
                             "&grant_type=client_credentials";
        try {
            json_lib response = execute_get(endpoint, query, false);
            if (!response.is_null() && response.contains("result") && response["result"].contains("access_token")) {
                return response["result"]["access_token"].get<string>();
            } else {
                cerr << "Error: Unable to find access_token in the response." << endl;
                return "";
            }
        } catch (const std::exception& e) {
            cerr << "Failed to get authentication token: " << e.what() << endl;
            return "";
        }  
    }

    string get_instrument_name(const string& currency) {
        const string endpoint = "public/get_instruments";
        const string query = "currency=" + currency;
        try {
            json_lib response = execute_get(endpoint, query, false);

            if (!response.is_null() && response.contains("result")) {
            for (const auto& instrument : response["result"]) {
                if (instrument.contains("instrument_name") && instrument["is_active"]) {
                    return instrument["instrument_name"].get<string>();
                }
            }
            cerr << "No active instrument found for the currency: " << currency << endl;
            return "";
        } else {
            cerr << "Error: Invalid response from get_instruments." << endl;
            return "";
        }
    } catch (const std::exception& e) {
        cerr << "Failed to get instrument name: " << e.what() << endl;
        return "";
    }
    }


    string place_order(const string& amount, const string& instrument_name){
        const string endpoint = "private/buy";
        const string query = "amount=" + amount +
                             "&instrument_name=" + instrument_name + "&type=market";
        try {
            json_lib response = execute_get(endpoint, query, true);
            if (!response.is_null() && response.contains("result")) {
                return response["result"].dump();
            } else {
                cerr << "Error: Unable to place order or invalid response." << endl;
                return "";
            }
        } catch (const std::exception& e) {
            cerr << "Failed to place order: " << e.what() << endl;
            return "";
        }
    }

    string sell_order(const float& amount, const string& instrument_name){
        const string endpoint = "private/sell";
        const string query = "amount=" + to_string(amount) +
                             "&instrument_name=" + instrument_name + "&price=" + to_string(1);
        try {
            json_lib response = execute_get(endpoint, query, true);
            if (!response.is_null() && response.contains("result")) {
                return response["result"].dump();
            } else {
                cerr << "Error: Unable to sell order or invalid response." << endl;
                return "";
            }
        } catch (const std::exception& e) {
            cerr << "Failed to sell order: " << e.what() << endl;
            return "";
        }
    }

    string cancel_order(const string& order_id){
        const string endpoint = "private/cancel";
        const string query = "order_id=" + order_id;
        try {
            json_lib response = execute_get(endpoint, query, true);
            if (!response.is_null() && response.contains("result")) {
                return response["result"].dump();
            } else {
                cerr << "Error: Unable to cancel order or invalid response." << endl;
                return "";
            }
        } catch (const std::exception& e) {
            cerr << "Failed to cancel order: " << e.what() << endl;
            return "";
        }
    }

    string modify_order(const string& order_id, const string& amount){
        const string endpoint = "private/edit";
        const string query = "order_id=" + order_id + "&amount=" + amount;
        try {
            json_lib response = execute_get(endpoint, query, true);
            if (!response.is_null() && response.contains("result")) {
                return response["result"].dump();
            } else {
                cerr << "Error: Unable to modify order or invalid response." << endl;
                return "";
            }
        } catch (const std::exception& e) {
            cerr << "Failed to modify order: " << e.what() << endl;
            return "";
        }   
    }

    string get_order_book(const string& instrument_name){
        const string endpoint = "public/get_order_book";
        const string query = "instrument_name=" + instrument_name + "&depth=5";

        try {
            json_lib response = execute_get(endpoint, query, false);
            if (!response.is_null() && response.contains("result")) {
                return response["result"].dump();
            } else {
                cerr << "Error: Unable to modify order or invalid response." << endl;
                return "";
            }
        } catch (const std::exception& e) {
                cerr << "Failed to modify order: " << e.what() << endl;
                return "";
        }

    }

    string view_current_positions(){
        const string endpoint = "private/get_open_orders";
        try {
            json_lib response = execute_get(endpoint,"", true);
            if (!response.is_null() && response.contains("result")) {
                return response["result"].dump();
            } else {
                cerr << "Error: Unable to view_current_positions or invalid response." << endl;
                return "";
            }
        } catch (const std::exception& e) {
                cerr << "Failed to view current positions: " << e.what() << endl;
                return "";
        }
    }
};

void display_menu() {
    cout << "\n--- Trading Menu ---\n";
    cout << "1. Submit Buy Order\n";
    cout << "2. Submit Sell Order\n";
    cout << "3. Update Existing Order\n";
    cout << "4. Cancel Order\n";
    cout << "5. View Order Book\n";
    cout << "6. Display Positions\n";
    cout << "0. Exit\n";
    cout << "Enter your choice: ";
}

void display_currency_choices() {
    cout << "Choose currency from the following options:\n";
    cout << "1. BTC\n";
    cout << "2. ETH\n";
    cout << "3. USDC\n";
    cout << "4. USDT\n";
    cout << "5. EURR\n";
    cout << "Enter your choice: ";
}

int main() {
    TradingClient client;

    while (true) {
        display_menu();
        int choice;
        cin >> choice;

        switch (choice) {
            case 1: {
                string amount, currency;
                display_currency_choices();
                int currency_choice;
                cin >> currency_choice;

                // Set the currency based on the user's choice
                switch (currency_choice) {
                    case 1: currency = "BTC"; break;
                    case 2: currency = "ETH"; break;
                    case 3: currency = "USDC"; break;
                    case 4: currency = "USDT"; break;
                    case 5: currency = "EURR"; break;
                    default:
                        cout << "Invalid choice, defaulting to BTC.\n";
                        currency = "BTC";
                        break;
                }

                cout << "Enter amount: ";
                cin >> amount;

                string instrument_name = client.get_instrument_name(currency);
                if (!instrument_name.empty()) {
                    string response = client.place_order(amount, instrument_name);
                    cout << "Buy Order Response: " << response << endl;
                } else {
                    cout << "Error: Instrument not found." << endl;
                }
                break;
            }

            case 2: {
                string currency;
                float amount;
                display_currency_choices();
                int currency_choice;
                cin >> currency_choice;

                // Set the currency based on the user's choice
                switch (currency_choice) {
                    case 1: currency = "BTC"; break;
                    case 2: currency = "ETH"; break;
                    case 3: currency = "USDC"; break;
                    case 4: currency = "USDT"; break;
                    case 5: currency = "EURR"; break;
                    default:
                        cout << "Invalid choice, defaulting to BTC.\n";
                        currency = "BTC";
                        break;
                }

                cout << "Enter amount: ";
                cin >> amount;

                string instrument_name = client.get_instrument_name(currency);
                if (!instrument_name.empty()) {
                    string response = client.sell_order(amount, instrument_name); // Sell order
                    cout << "Sell Order Response: " << response << endl;
                } else {
                    cout << "Error: Instrument not found." << endl;
                }
                break;
            }

            case 3: {
                string order_id, amount;
                cout << "Enter order ID: ";
                cin >> order_id;
                cout << "Enter new amount: ";
                cin >> amount;

                string response = client.modify_order(order_id, amount);
                cout << "Modify Order Response: " << response << endl;
                break;
            }

            case 4: {
                string order_id;
                cout << "Enter order ID: ";
                cin >> order_id;

                string response = client.cancel_order(order_id);
                cout << "Cancel Order Response: " << response << endl;
                break;
            }

            case 5: {
                string currency;
                display_currency_choices();
                int currency_choice;
                cin >> currency_choice;

                // Set the currency based on the user's choice
                switch (currency_choice) {
                    case 1: currency = "BTC"; break;
                    case 2: currency = "ETH"; break;
                    case 3: currency = "USDC"; break;
                    case 4: currency = "USDT"; break;
                    case 5: currency = "EURR"; break;
                    case 6: 
                        cout << "Enter custom currency: ";
                        cin >> currency;
                        break;
                    default:
                        cout << "Invalid choice, defaulting to BTC.\n";
                        currency = "BTC";
                        break;
                }

                string instrument_name = client.get_instrument_name(currency);
                if (!instrument_name.empty()) {
                    string order_book = client.get_order_book(instrument_name);
                    cout << "Order Book: " << order_book << endl;
                } else {
                    cout << "Error: Instrument not found." << endl;
                }
                break;
            }

            case 6: {
                string positions = client.view_current_positions();
                cout << "Current Positions: " << positions << endl;
                break;
            }

            case 0:
                cout << "Exiting the program." << endl;
                return 0;

            default:
                cout << "Invalid choice. Please try again." << endl;
        }
    }

    return 0;
}


