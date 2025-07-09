#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <vector>
#include <winsock2.h>
#include <thread>
#include <mutex>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <algorithm>

#pragma comment(lib, "ws2_32.lib")
#define PORT 8080

using namespace std;

mutex walletMutex; // Thread safety for wallet operations
mutex transactionMutex; // Thread safety for transaction history

struct Transaction {
    string timestamp;
    string type;
    double amount;
    double balanceAfter;
    
    string toString() const {
        return timestamp + "|" + type + "|" + to_string(amount) + "|" + to_string(balanceAfter);
    }
    
    static Transaction fromString(const string& str) {
        Transaction t;
        stringstream ss(str);
        string item;
        
        getline(ss, t.timestamp, '|');
        getline(ss, t.type, '|');
        getline(ss, item, '|');
        t.amount = stod(item);
        getline(ss, item, '|');
        t.balanceAfter = stod(item);
        
        return t;
    }
};

map<string, pair<string, double>> loadWallets(const string& filename) {
    map<string, pair<string, double>> wallets;
    ifstream file(filename);
    string user, pin;
    double balance;

    while (file >> user >> pin >> balance) {
        wallets[user] = {pin, balance};
    }
    return wallets;
}

void saveWallets(const string& filename, const map<string, pair<string, double>>& wallets) {
    ofstream file(filename);
    for (const auto& entry : wallets) {
        file << entry.first << " " << entry.second.first << " " << entry.second.second << endl;
    }
}

map<string, vector<Transaction>> loadTransactionHistory() {
    map<string, vector<Transaction>> history;
    ifstream file("transactions.txt");
    string line;
    
    while (getline(file, line)) {
        if (line.empty()) continue;
        
        size_t colonPos = line.find(':');
        if (colonPos != string::npos) {
            string username = line.substr(0, colonPos);
            string transactionStr = line.substr(colonPos + 1);
            
            if (!transactionStr.empty()) {
                Transaction t = Transaction::fromString(transactionStr);
                history[username].push_back(t);
            }
        }
    }
    return history;
}

void saveTransactionHistory(const map<string, vector<Transaction>>& history) {
    ofstream file("transactions.txt");
    for (const auto& userHistory : history) {
        const string& username = userHistory.first;
        for (const auto& transaction : userHistory.second) {
            file << username << ":" << transaction.toString() << endl;
        }
    }
}

void addTransaction(map<string, vector<Transaction>>& history, const string& username, 
                   const string& type, double amount, double newBalance) {
    lock_guard<mutex> lock(transactionMutex);
    
    Transaction t;
    
    // Get current time
    time_t now = time(0);
    tm* timeinfo = localtime(&now);
    stringstream ss;
    ss << put_time(timeinfo, "%Y-%m-%d %H:%M:%S");
    t.timestamp = ss.str();
    
    t.type = type;
    t.amount = amount;
    t.balanceAfter = newBalance;
    
    history[username].push_back(t);
    saveTransactionHistory(history);
}

string getCurrentTimestamp() {
    time_t now = time(0);
    tm* timeinfo = localtime(&now);
    stringstream ss;
    ss << put_time(timeinfo, "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

void handleClient(SOCKET clientSocket, map<string, pair<string, double>>& wallets, 
                 map<string, vector<Transaction>>& transactionHistory) {
    cout << "New client connected. Thread ID: " << this_thread::get_id() << endl;
    
    char buffer[2048];
    string username, pin;
    
    // First, ask if user wants to login or register
    send(clientSocket, "LOGIN_OR_REGISTER", 17, 0);
    
    memset(buffer, 0, sizeof(buffer));
    recv(clientSocket, buffer, sizeof(buffer), 0);
    string choice(buffer);
    
    if (choice == "REGISTER") {
        // Registration process
        send(clientSocket, "ENTER_NEW_USERNAME", 18, 0);
        
        memset(buffer, 0, sizeof(buffer));
        recv(clientSocket, buffer, sizeof(buffer), 0);
        username = string(buffer);
        
        // Check if username already exists
        lock_guard<mutex> lock(walletMutex);
        if (wallets.find(username) != wallets.end()) {
            send(clientSocket, "USERNAME_EXISTS", 15, 0);
            closesocket(clientSocket);
            return;
        }
        
        send(clientSocket, "ENTER_NEW_PIN", 13, 0);
        
        memset(buffer, 0, sizeof(buffer));
        recv(clientSocket, buffer, sizeof(buffer), 0);
        pin = string(buffer);
        
        // Validate PIN (must be 4 digits)
        if (pin.length() != 4 || !all_of(pin.begin(), pin.end(), ::isdigit)) {
            send(clientSocket, "INVALID_PIN_FORMAT", 18, 0);
            closesocket(clientSocket);
            return;
        }
        
        send(clientSocket, "ENTER_INITIAL_BALANCE", 21, 0);
        
        memset(buffer, 0, sizeof(buffer));
        recv(clientSocket, buffer, sizeof(buffer), 0);
        double initialBalance;
        try {
            initialBalance = stod(string(buffer));
            if (initialBalance < 0) {
                send(clientSocket, "NEGATIVE_BALANCE", 16, 0);
                closesocket(clientSocket);
                return;
            }
        } catch (const exception& e) {
            send(clientSocket, "INVALID_BALANCE_FORMAT", 22, 0);
            closesocket(clientSocket);
            return;
        }
        
        // Create new user
        wallets[username] = {pin, initialBalance};
        saveWallets("wallets.txt", wallets);
        
        // Add initial transaction
        addTransaction(transactionHistory, username, "ACCOUNT_CREATED", initialBalance, initialBalance);
        
        send(clientSocket, "REGISTRATION_SUCCESS", 20, 0);
        cout << "New user registered: " << username << " with balance: $" << initialBalance << endl;
    }
    else if (choice == "LOGIN") {
        // Login process
        send(clientSocket, "ENTER_USERNAME", 14, 0);
        
        memset(buffer, 0, sizeof(buffer));
        recv(clientSocket, buffer, sizeof(buffer), 0);
        username = string(buffer);
        
        send(clientSocket, "ENTER_PIN", 9, 0);
        
        memset(buffer, 0, sizeof(buffer));
        recv(clientSocket, buffer, sizeof(buffer), 0);
        pin = string(buffer);
        
        // Authentication
        lock_guard<mutex> lock(walletMutex);
        if (wallets.find(username) == wallets.end() || wallets[username].first != pin) {
            send(clientSocket, "AUTH_FAILED", 11, 0);
            cout << "Authentication failed for user: " << username << endl;
            closesocket(clientSocket);
            return;
        }
        
        send(clientSocket, "AUTH_SUCCESS", 12, 0);
        cout << "User " << username << " authenticated successfully." << endl;
    }
    else {
        send(clientSocket, "INVALID_CHOICE", 14, 0);
        closesocket(clientSocket);
        return;
    }
    
    // Handle commands after successful login/registration
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int bytes = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytes <= 0) break;
        
        string request(buffer);
        cout << "Command from " << username << ": " << request << endl;
        
        if (request == "BALANCE") {
            lock_guard<mutex> lock(walletMutex);
            double bal = wallets[username].second;
            string msg = "Balance: $" + to_string(bal);
            send(clientSocket, msg.c_str(), msg.size(), 0);
        } 
        else if (request.rfind("ADD ", 0) == 0) {
            try {
                double amount = stod(request.substr(4));
                if (amount > 0) {
                    lock_guard<mutex> lock(walletMutex);
                    wallets[username].second += amount;
                    double newBalance = wallets[username].second;
                    saveWallets("wallets.txt", wallets);
                    
                    addTransaction(transactionHistory, username, "DEPOSIT", amount, newBalance);
                    
                    string msg = "Added $" + to_string(amount) + ". New balance: $" + to_string(newBalance);
                    send(clientSocket, msg.c_str(), msg.size(), 0);
                } else {
                    send(clientSocket, "Invalid amount. Must be positive.", 33, 0);
                }
            } catch (const exception& e) {
                send(clientSocket, "Invalid amount format.", 22, 0);
            }
        }
        else if (request.rfind("WITHDRAW ", 0) == 0) {
            try {
                double amount = stod(request.substr(9));
                if (amount > 0) {
                    lock_guard<mutex> lock(walletMutex);
                    if (wallets[username].second >= amount) {
                        wallets[username].second -= amount;
                        double newBalance = wallets[username].second;
                        saveWallets("wallets.txt", wallets);
                        
                        addTransaction(transactionHistory, username, "WITHDRAWAL", amount, newBalance);
                        
                        string msg = "Withdrawn $" + to_string(amount) + ". New balance: $" + to_string(newBalance);
                        send(clientSocket, msg.c_str(), msg.size(), 0);
                    } else {
                        send(clientSocket, "Insufficient balance.", 21, 0);
                    }
                } else {
                    send(clientSocket, "Invalid amount. Must be positive.", 33, 0);
                }
            } catch (const exception& e) {
                send(clientSocket, "Invalid amount format.", 22, 0);
            }
        }
        else if (request == "HISTORY") {
            lock_guard<mutex> lock(transactionMutex);
            string response = "TRANSACTION_HISTORY_START\n";
            
            if (transactionHistory.find(username) != transactionHistory.end()) {
                for (const auto& transaction : transactionHistory[username]) {
                    response += transaction.timestamp + "|" + transaction.type + "|$" + 
                               to_string(transaction.amount) + "|Balance: $" + 
                               to_string(transaction.balanceAfter) + "\n";
                }
            } else {
                response += "No transactions found.\n";
            }
            response += "TRANSACTION_HISTORY_END";
            
            send(clientSocket, response.c_str(), response.size(), 0);
        }
        else if (request == "EXIT") {
            break;
        } 
        else {
            send(clientSocket, "Invalid command. Available: BALANCE, ADD <amount>, WITHDRAW <amount>, HISTORY, EXIT", 82, 0);
        }
    }
    
    cout << "Client " << username << " disconnected." << endl;
    closesocket(clientSocket);
}

int main() {
    WSADATA wsa;
    SOCKET serverSocket;
    sockaddr_in serverAddr;
    
    WSAStartup(MAKEWORD(2, 2), &wsa);
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    
    bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));
    listen(serverSocket, 5);
    
    cout << "=== Digital Wallet Server Started ===" << endl;
    cout << "Listening on port " << PORT << endl;
    cout << "Features: Login, Registration, Transaction History" << endl;
    cout << "Waiting for clients..." << endl;
    
    map<string, pair<string, double>> wallets = loadWallets("wallets.txt");
    map<string, vector<Transaction>> transactionHistory = loadTransactionHistory();
    
    while (true) {
        sockaddr_in clientAddr;
        int clientLen = sizeof(clientAddr);
        SOCKET clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientLen);
        
        if (clientSocket != INVALID_SOCKET) {
            thread clientThread(handleClient, clientSocket, ref(wallets), ref(transactionHistory));
            clientThread.detach();
        }
    }
    
    closesocket(serverSocket);
    WSACleanup();
    return 0;
}