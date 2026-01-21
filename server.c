#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "common/protocol.h"
#include "common/logger.h"

// --- HELPER: Get Current Balance ---
int get_current_balance(char *username) {
    char filename[100];
    sprintf(filename, "database/customers/%s.txt", username);
    FILE *fp = fopen(filename, "r");
    if (!fp) return -1; // File not found

    int bal = 0;
    char line[100];
    char trans[50];
    int amt, temp_bal;

    // Read every line; the last valid balance found is the current one
    while (fgets(line, sizeof(line), fp)) {
        // Parse: TransactionName Amount Balance
        sscanf(line, "%s %d %d", trans, &amt, &temp_bal);
        bal = temp_bal;
    }
    fclose(fp);
    return bal;
}

// --- HANDLER: Registration ---
void handle_register(int client_sock, Message msg) {
    Message response; 
    memset(&response, 0, sizeof(response));

    // Check if user exists
    FILE *fp = fopen("database/login.txt", "r");
    char u[50], p[50], r;
    int exists = 0;
    if (fp) {
        while (fscanf(fp, "%s %s %c", u, p, &r) != EOF) {
            if (strcmp(u, msg.username) == 0) exists = 1;
        }
        fclose(fp);
    }

    if (exists) {
        response.type = MSG_ERROR;
        strcpy(response.data, "Error: Username already exists.");
    } else {
        // Save to login.txt
        fp = fopen("database/login.txt", "a");
        fprintf(fp, "%s %s %c\n", msg.username, msg.password, msg.role);
        fclose(fp);

        // Create Account File
        if (msg.role == 'C') {
            char filename[100];
            sprintf(filename, "database/customers/%s.txt", msg.username);
            fp = fopen(filename, "w");
            fprintf(fp, "Account_Opened %d %d\n", msg.amount, msg.amount);
            fclose(fp);
        }
        response.type = MSG_SUCCESS;
        strcpy(response.data, "Registration Successful.");
        write_log("New User Registered.");
    }
    send(client_sock, &response, sizeof(response), 0);
}

// --- HANDLER: Login ---
void handle_login(int client_sock, Message msg) {
    Message response;
    memset(&response, 0, sizeof(response));
    
    FILE *fp = fopen("database/login.txt", "r");
    char u[50], p[50], r;
    int authenticated = 0;

    if (fp) {
        while (fscanf(fp, "%s %s %c", u, p, &r) != EOF) {
            // Check matching Username AND Password
            if (strcmp(u, msg.username) == 0 && strcmp(p, msg.password) == 0) {
                authenticated = 1;
                response.role = r; // Send role back to client
                break;
            }
        }
        fclose(fp);
    }

    if (authenticated) {
        response.type = MSG_SUCCESS;
        strcpy(response.data, "Login Successful.");
        
        char log_msg[100];
        sprintf(log_msg, "User logged in: %s", msg.username);
        write_log(log_msg);
    } else {
        response.type = MSG_ERROR;
        strcpy(response.data, "Invalid Username or Password.");
        write_log("Failed login attempt.");
    }
    send(client_sock, &response, sizeof(response), 0);
}

// --- HANDLER: Transactions (Deposit/Withdraw) ---
void handle_transaction(int client_sock, Message msg) {
    Message response;
    memset(&response, 0, sizeof(response));

    int current_bal = get_current_balance(msg.username);
    
    if (current_bal == -1) {
        response.type = MSG_ERROR;
        strcpy(response.data, "Account file error.");
        send(client_sock, &response, sizeof(response), 0);
        return;
    }

    int new_bal = current_bal;
    char trans_type[50];

    if (msg.type == MSG_DEPOSIT) {
        new_bal += msg.amount;
        strcpy(trans_type, "Deposit");
    } else if (msg.type == MSG_WITHDRAW) {
        if (current_bal < msg.amount) {
            response.type = MSG_ERROR;
            strcpy(response.data, "Insufficient Funds.");
            send(client_sock, &response, sizeof(response), 0);
            return;
        }
        new_bal -= msg.amount;
        strcpy(trans_type, "Withdrawal");
    }

    // Append transaction to file
    char filename[100];
    sprintf(filename, "database/customers/%s.txt", msg.username);
    FILE *fp = fopen(filename, "a");
    fprintf(fp, "%s %d %d\n", trans_type, msg.amount, new_bal);
    fclose(fp);

    response.type = MSG_SUCCESS;
    sprintf(response.data, "Transaction Complete. New Balance: %d", new_bal);
    
    // Log it
    char log_msg[100];
    sprintf(log_msg, "User %s %s amount %d. New Bal: %d", msg.username, trans_type, msg.amount, new_bal);
    write_log(log_msg);

    send(client_sock, &response, sizeof(response), 0);
}

// --- MAIN LOOP ---
void process_client(int client_sock) {
    Message msg;
    // We loop here to allow multiple transactions in one session
    while (recv(client_sock, &msg, sizeof(msg), 0) > 0) {
        printf("Received Request Type: %d from %s\n", msg.type, msg.username);

        switch (msg.type) {
            case MSG_REGISTER:
                handle_register(client_sock, msg);
                break;
            case MSG_LOGIN:
                handle_login(client_sock, msg);
                break;
            case MSG_DEPOSIT:
            case MSG_WITHDRAW:
                handle_transaction(client_sock, msg);
                break;
            default:
                break;
        }
    }
    close(client_sock);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        exit(1);
    }
    int port = atoi(argv[1]);
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size;

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    // Enable address reuse to avoid "Address already in use" errors
    int opt = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr));
    listen(server_sock, 5);

    printf("Server listening on port %d...\n", port);
    write_log("Server Started.");

    while (1) {
        addr_size = sizeof(client_addr);
        client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &addr_size);
        if (client_sock < 0) continue;
        
        // Handle client in a loop
        if (fork() == 0) { // Child process
            close(server_sock);
            process_client(client_sock);
            exit(0);
        }
        close(client_sock); // Parent closes client socket
    }
    return 0;
}