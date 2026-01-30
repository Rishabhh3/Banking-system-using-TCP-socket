# 🏦 Concurrent Banking System (C Socket Programming)

A robust, multi-process Client-Server banking application built in C using TCP sockets. This system simulates a real-world banking environment with Role-Based Access Control (RBAC), supporting concurrent users via the `fork()` system call.

##  Features

### Core Architecture
* **Client-Server Model:** Separates the user interface (Client) from business logic and data storage (Server).
* **Concurrency:** Handles multiple simultaneous client connections using `fork()`.
* **TCP/IP Communication:** Uses custom protocol to ensure reliable data transmission.
* **Persistence:** File-based database system for storing user credentials and transaction history.

### 🔐 Role-Based Access Control (RBAC)
The system supports three distinct user roles with specific permissions:

1.  **Administrator (Admin)**
    * **User Management:** Add new users (Customer, Police, or Admin) and delete existing users.
    * **Financial Control:** Credit or Debit any customer account directly.
    * **Auditing:** View account balance of any user.
    * *Security:* Uses a dedicated protocol channel (`MSG_ADMIN_ADD_USER`) to prevent unauthorized privilege escalation.

2.  **Customer**
    * **Transactions:** Deposit and Withdraw funds.
    * **Account Info:** View current balance.
    * **History:** View a mini-statement of recent transactions.
    * *Security:* Can only access their own data.

3.  **Police**
    * **Auditing:** Read-only access to inspect any customer's balance.
    * **Investigation:** View mini-statements of specific customers.
    * *Restriction:* Cannot modify balances or delete users.

---

## 🛠️ Tech Stack

* **Language:** C (Standard C11)
* **Networking:** BSD Sockets (`<sys/socket.h>`, `<netinet/in.h>`)
* **Process Management:** Linux System Calls (`fork()`, `exit()`, `wait()`)
* **File I/O:** Standard C File Handling for database persistence.
* **OS:** Linux / macOS (Unix-based systems).

---
# To run server : gcc server.c common/logger.c -o server
and then ./server 8080

# To run client : gcc client.c -o client
and then ./client 127.0.0.1 8080

```text
.
├── server.c             # Main entry point for the Server
├── client.c             # Main entry point for the Client
├── common/              # Shared resources
│   ├── protocol.h       # Protocol definitions (Message struct, Enums)
│   ├── logger.c         # implementation of logging functions
│   └── logger.h         # Header for logging
└── database/            # Database management
    ├── db_setup.c       # Functions to initialize DB and folders
    ├── login.txt        # User credentials (created automatically/manually)
    └── customers/       # Transaction history files



