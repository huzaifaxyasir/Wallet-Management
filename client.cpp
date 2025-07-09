#include <iostream>
#include <winsock2.h>
#include <string>
#include <iomanip>
#include <conio.h>
#include <windows.h>
#include <sstream>
#include <vector>

#pragma comment(lib, "ws2_32.lib")
#define PORT 8080

using namespace std;

// Function to set console text color
void setColor(int color) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
}

// Function to clear screen
void clearScreen() {
    system("cls");
}

// Function to display banner
void displayBanner() {
    setColor(11); // Light Cyan
    cout << "========================================" << endl;
    cout << "       DIGITAL WALLET CLIENT           " << endl;
    cout << "========================================" << endl;
    setColor(7); // White
}

// Function to display main menu
void displayMenu() {
    setColor(14); // Light Yellow
    cout << "\n+------ WALLET OPERATIONS ------+" << endl;
    cout << "| 1. Check Balance               |" << endl;
    cout << "| 2. Add Money                   |" << endl;
    cout << "| 3. Withdraw Money              |" << endl;
    cout << "| 4. Transaction History         |" << endl;
    cout << "| 5. Account Information         |" << endl;
    cout << "| 6. Exit                        |" << endl;
    cout << "+--------------------------------+" << endl;
    setColor(7);
}

// Function to display login/register menu
void displayLoginMenu() {
    setColor(14); // Light Yellow
    cout << "\n+------ WELCOME ------+" << endl;
    cout << "| 1. Login             |" << endl;
    cout << "| 2. Register New User |" << endl;
    cout << "+----------------------+" << endl;
    setColor(7);
}

// Function to get password input (hidden)
string getPassword(const string& prompt) {
    string password;
    char ch;
    cout << prompt;
    while ((ch = _getch()) != '\r') { // '\r' is Enter key
        if (ch == '\b') { // Backspace
            if (!password.empty()) {
                password.pop_back();
                cout << "\b \b";
            }
        } else {
            password += ch;
            cout << "*";
        }
    }
    cout << endl;
    return password;
}

// Function to validate amount input
bool isValidAmount(const string& input, double& amount) {
    try {
        amount = stod(input);
        return amount > 0;
    } catch (const exception&) {
        return false;
    }
}

// Function to display loading animation
void showLoading(const string& message) {
    cout << message;
    for (int i = 0; i < 3; i++) {
        cout << ".";
        Sleep(500);
    }
    cout << endl;
}

// Function to validate PIN format
bool isValidPIN(const string& pin) {
    if (pin.length() != 4) return false;
    for (char c : pin) {
        if (!isdigit(c)) return false;
    }
    return true;
}

// Function to display transaction history
void displayTransactionHistory(const string& historyData) {
    clearScreen();
    displayBanner();
    setColor(11);
    cout << "=== TRANSACTION HISTORY ===" << endl;
    setColor(7);
    
    stringstream ss(historyData);
    string line;
    bool startFound = false;
    int transactionCount = 0;
    
    while (getline(ss, line)) {
        if (line == "TRANSACTION_HISTORY_START") {
            startFound = true;
            continue;
        }
        if (line == "TRANSACTION_HISTORY_END") {
            break;
        }
        if (startFound && !line.empty()) {
            if (line == "No transactions found.") {
                setColor(8);
                cout << line << endl;
                setColor(7);
            } else {
                // Parse transaction line: timestamp|type|amount|balance
                size_t pos1 = line.find('|');
                size_t pos2 = line.find('|', pos1 + 1);
                size_t pos3 = line.find('|', pos2 + 1);
                
                if (pos1 != string::npos && pos2 != string::npos && pos3 != string::npos) {
                    string timestamp = line.substr(0, pos1);
                    string type = line.substr(pos1 + 1, pos2 - pos1 - 1);
                    string amount = line.substr(pos2 + 1, pos3 - pos2 - 1);
                    string balance = line.substr(pos3 + 1);
                    
                    transactionCount++;
                    
                    // Color coding based on transaction type
                    if (type == "DEPOSIT") setColor(10); // Green
                    else if (type == "WITHDRAWAL") setColor(12); // Red
                    else if (type == "ACCOUNT_CREATED") setColor(11); // Cyan
                    else setColor(7); // White
                    
                    cout << "[" << transactionCount << "] " << timestamp << " - " << type << endl;
                    cout << "    Amount: " << amount << " | " << balance << endl;
                    setColor(8);
                    cout << "    " << string(50, '-') << endl;
                    setColor(7);
                }
            }
        }
    }
    
    if (transactionCount > 0) {
        setColor(14);
        cout << "\nTotal Transactions: " << transactionCount << endl;
        setColor(7);
    }
}

int main() {
    WSADATA wsa;
    SOCKET clientSocket;
    sockaddr_in serverAddr;
    
    WSAStartup(MAKEWORD(2, 2), &wsa);
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    
    clearScreen();
    displayBanner();
    
    setColor(10); // Light Green
    cout << "Connecting to server";
    showLoading("");
    
    if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        setColor(12); // Light Red
        cout << "Failed to connect to server!" << endl;
        setColor(7);
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }
    
    setColor(10);
    cout << "Connected to Digital Wallet Server!" << endl;
    setColor(7);
    
    char recvBuf[2048];
    string username;
    
    // Login or Register choice
    displayLoginMenu();
    string loginChoice;
    do {
        cout << "\nSelect option (1-2): ";
        getline(cin, loginChoice);
        
        if (loginChoice != "1" && loginChoice != "2") {
            setColor(12);
            cout << "Invalid option. Please select 1 or 2." << endl;
            setColor(7);
        }
    } while (loginChoice != "1" && loginChoice != "2");
    
    // Wait for server prompt
    recv(clientSocket, recvBuf, sizeof(recvBuf), 0);
    
    if (loginChoice == "2") {
        // Registration
        clearScreen();
        displayBanner();
        setColor(11);
        cout << "=== USER REGISTRATION ===" << endl;
        setColor(7);
        
        send(clientSocket, "REGISTER", 8, 0);
        
        // Get new username
        recv(clientSocket, recvBuf, sizeof(recvBuf), 0);
        cout << "Enter new username: ";
        getline(cin, username);
        send(clientSocket, username.c_str(), username.size(), 0);
        
        // Check server response
        memset(recvBuf, 0, sizeof(recvBuf));
        recv(clientSocket, recvBuf, sizeof(recvBuf), 0);
        string response(recvBuf);
        
        if (response == "USERNAME_EXISTS") {
            setColor(12);
            cout << "Username already exists! Please try again later." << endl;
            setColor(7);
            closesocket(clientSocket);
            WSACleanup();
            return 1;
        }
        
        // Get new PIN
        string pin;
        do {
            pin = getPassword("Enter 4-digit PIN: ");
            if (!isValidPIN(pin)) {
                setColor(12);
                cout << "PIN must be exactly 4 digits!" << endl;
                setColor(7);
            }
        } while (!isValidPIN(pin));
        
        send(clientSocket, pin.c_str(), pin.size(), 0);
        
        // Check PIN format response
        memset(recvBuf, 0, sizeof(recvBuf));
        recv(clientSocket, recvBuf, sizeof(recvBuf), 0);
        response = string(recvBuf);
        
        if (response == "INVALID_PIN_FORMAT") {
            setColor(12);
            cout << "Invalid PIN format!" << endl;
            setColor(7);
            closesocket(clientSocket);
            WSACleanup();
            return 1;
        }
        
        // Get initial balance
        string balanceStr;
        double balance;
        do {
            cout << "Enter initial balance: $";
            getline(cin, balanceStr);
            
            if (!isValidAmount(balanceStr, balance)) {
                setColor(12);
                cout << "Invalid amount. Please enter a positive number." << endl;
                setColor(7);
            }
        } while (!isValidAmount(balanceStr, balance));
        
        send(clientSocket, balanceStr.c_str(), balanceStr.size(), 0);
        
        // Check registration result
        memset(recvBuf, 0, sizeof(recvBuf));
        recv(clientSocket, recvBuf, sizeof(recvBuf), 0);
        response = string(recvBuf);
        
        if (response == "REGISTRATION_SUCCESS") {
            setColor(10);
            cout << "\nRegistration successful! Welcome, " << username << "!" << endl;
            setColor(7);
        } else {
            setColor(12);
            cout << "Registration failed: " << response << endl;
            setColor(7);
            closesocket(clientSocket);
            WSACleanup();
            return 1;
        }
    }
    else {
        // Login
        clearScreen();
        displayBanner();
        setColor(11);
        cout << "=== USER LOGIN ===" << endl;
        setColor(7);
        
        send(clientSocket, "LOGIN", 5, 0);
        
        // Get username
        recv(clientSocket, recvBuf, sizeof(recvBuf), 0);
        cout << "Enter Username: ";
        getline(cin, username);
        send(clientSocket, username.c_str(), username.size(), 0);
        
        // Get PIN
        recv(clientSocket, recvBuf, sizeof(recvBuf), 0);
        string pin = getPassword("Enter PIN: ");
        send(clientSocket, pin.c_str(), pin.size(), 0);
        
        // Check authentication
        memset(recvBuf, 0, sizeof(recvBuf));
        recv(clientSocket, recvBuf, sizeof(recvBuf), 0);
        string response(recvBuf);
        
        if (response != "AUTH_SUCCESS") {
            setColor(12);
            cout << "\nAuthentication failed!" << endl;
            cout << "Please check your username and PIN." << endl;
            setColor(7);
            closesocket(clientSocket);
            WSACleanup();
            return 1;
        }
        
        setColor(10);
        cout << "\nLogin successful! Welcome back, " << username << "!" << endl;
        setColor(7);
    }
    
    // Main application loop
    while (true) {
        displayMenu();
        
        cout << "\nSelect option (1-6): ";
        string choice;
        getline(cin, choice);
        
        if (choice == "1") {
            // Check Balance
            clearScreen();
            displayBanner();
            setColor(11);
            cout << "=== BALANCE INQUIRY ===" << endl;
            setColor(7);
            showLoading("Fetching balance");
            
            send(clientSocket, "BALANCE", 7, 0);
            memset(recvBuf, 0, sizeof(recvBuf));
            recv(clientSocket, recvBuf, sizeof(recvBuf), 0);
            
            setColor(10);
            cout << ">> " << recvBuf << endl;
            setColor(7);
        }
        else if (choice == "2") {
            // Add Money
            clearScreen();
            displayBanner();
            setColor(11);
            cout << "=== ADD MONEY ===" << endl;
            setColor(7);
            
            string amountStr;
            double amount;
            
            do {
                cout << "Enter amount to add: $";
                getline(cin, amountStr);
                
                if (!isValidAmount(amountStr, amount)) {
                    setColor(12);
                    cout << "Invalid amount. Please enter a positive number." << endl;
                    setColor(7);
                }
            } while (!isValidAmount(amountStr, amount));
            
            showLoading("Processing transaction");
            
            string command = "ADD " + amountStr;
            send(clientSocket, command.c_str(), command.size(), 0);
            
            memset(recvBuf, 0, sizeof(recvBuf));
            recv(clientSocket, recvBuf, sizeof(recvBuf), 0);
            
            setColor(10);
            cout << ">> " << recvBuf << endl;
            setColor(7);
        }
        else if (choice == "3") {
            // Withdraw Money
            clearScreen();
            displayBanner();
            setColor(11);
            cout << "=== WITHDRAW MONEY ===" << endl;
            setColor(7);
            
            string amountStr;
            double amount;
            
            do {
                cout << "Enter amount to withdraw: $";
                getline(cin, amountStr);
                
                if (!isValidAmount(amountStr, amount)) {
                    setColor(12);
                    cout << "Invalid amount. Please enter a positive number." << endl;
                    setColor(7);
                }
            } while (!isValidAmount(amountStr, amount));
            
            showLoading("Processing withdrawal");
            
            string command = "WITHDRAW " + amountStr;
            send(clientSocket, command.c_str(), command.size(), 0);
            
            memset(recvBuf, 0, sizeof(recvBuf));
            recv(clientSocket, recvBuf, sizeof(recvBuf), 0);
            
            if (string(recvBuf).find("Insufficient") != string::npos) {
                setColor(12);
            } else {
                setColor(10);
            }
            cout << ">> " << recvBuf << endl;
            setColor(7);
        }
        else if (choice == "4") {
            // Transaction History
            showLoading("Fetching transaction history");
            
            send(clientSocket, "HISTORY", 7, 0);
            
            memset(recvBuf, 0, sizeof(recvBuf));
            recv(clientSocket, recvBuf, sizeof(recvBuf), 0);
            
            displayTransactionHistory(string(recvBuf));
        }
        else if (choice == "5") {
            // Account Information
            clearScreen();
            displayBanner();
            setColor(11);
            cout << "=== ACCOUNT INFORMATION ===" << endl;
            setColor(7);
            
            cout << "Account Holder: " << username << endl;
            cout << "Account Type: Digital Wallet" << endl;
            cout << "Status: Active" << endl;
            
            // Get current balance
            send(clientSocket, "BALANCE", 7, 0);
            memset(recvBuf, 0, sizeof(recvBuf));
            recv(clientSocket, recvBuf, sizeof(recvBuf), 0);
            cout << "Current " << recvBuf << endl;
        }
        else if (choice == "6") {
            // Exit
            setColor(14);
            cout << "\nThank you for using Digital Wallet!" << endl;
            cout << "Goodbye, " << username << "!" << endl;
            setColor(7);
            
            send(clientSocket, "EXIT", 4, 0);
            break;
        }
        else {
            setColor(12);
            cout << "Invalid option. Please select 1-6." << endl;
            setColor(7);
        }
        
        if (choice != "6") {
            cout << "\nPress Enter to continue...";
            cin.get();
            clearScreen();
            displayBanner();
        }
    }
    
    closesocket(clientSocket);
    WSACleanup();
    return 0;
}