# 💰 Client-Server Wallet Management System

A secure, multi-threaded Wallet Management System implemented in **C++** using **Windows Sockets**. This project demonstrates how to build a robust client-server architecture that supports user authentication and real-time transaction management.

---

## 📌 Features

✅ **User Registration & Login**  
- Securely create accounts and authenticate users.

✅ **Balance Inquiry**  
- View current wallet balance.

✅ **Deposit & Withdrawal**  
- Perform deposits and withdrawals with real-time updates.

✅ **Transaction History**  
- View a complete record of all transactions per user.

✅ **Multi-Client Support**  
- Handles multiple clients simultaneously using multithreading.

✅ **Socket Programming**  
- Client and server communicate via Windows TCP sockets.

---

## ⚙️ Tech Stack

- **Language:** C++
- **Networking:** Windows Sockets API (Winsock)
- **Concurrency:** Multithreading with `std::thread`
- **Platform:** Windows

---

## 🚀 How to Run

### 🖥️ Server

1. Open the `Server` project folder.
2. Compile the server code:
   ```bash
   g++ server.cpp -o server -lws2_32
