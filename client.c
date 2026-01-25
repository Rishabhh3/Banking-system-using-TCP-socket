#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "common/protocol.h"

int sock;

// --- SHARED HELPER: Send & Print ---
void send_request(Message msg) {
    Message response;
    send(sock, &msg, sizeof(msg), 0);
    recv(sock, &response, sizeof(response), 0);
    printf("\n[SERVER]: %s\n", response.data);
}

void customer_menu(char *username) {
    int choice;
    while (1) {
        Message msg = {0};
        strcpy(msg.username, username);
        msg.role = 'C';

        printf("\n--- CUSTOMER MENU ---\n1. Deposit\n2. Withdraw\n3. Balance\n4. Statement\n5. Logout\nChoice: ");
        scanf("%d", &choice);
        if (choice == 5) break;

        if (choice == 1 || choice == 2) {
            msg.type = (choice == 1) ? MSG_DEPOSIT : MSG_WITHDRAW;
            printf("Amount: "); scanf("%d", &msg.amount);
        } else if (choice == 3) msg.type = MSG_BALANCE;
        else if (choice == 4) msg.type = MSG_MINI_STATEMENT;
        
        send_request(msg);
    }
}

// client.c

// Change from (int sock) to (char *username)
void admin_menu(char *username) {  
    int choice;
    Message msg = {0}; // Initialize empty
    
    // Copy the username so the server knows who is asking
    strcpy(msg.username, username);
    msg.role = 'A'; 
    
    while(1) {
        printf("\n--- ADMIN MENU ---\n");
        printf("1. Add User\n2. Delete User\n3. Credit\n4. Debit\n5. Check Balance\n6. Exit\nChoice: ");
        scanf("%d", &choice);

        if (choice == 6) break;

        // Reset message for new request, but keep identity
        memset(&msg, 0, sizeof(msg));
        strcpy(msg.username, username);
        msg.role = 'A';

        switch(choice) {
            case 1: // Add User
                msg.type = MSG_REGISTER;
                printf("New Username: "); scanf("%s", msg.username); // Be careful! This overwrites msg.username. 
             
                printf("Password: "); scanf("%s", msg.password);
                printf("Role (C/P/A): "); scanf(" %c", &msg.role);
                if(msg.role == 'C') { printf("Amount: "); scanf("%d", &msg.amount); }
                break;

            case 2: // Delete User
                msg.type = MSG_DELETE_USER;
                printf("User to delete: "); scanf("%s", msg.target_username);
                break;
            
            case 3: // Credit
                msg.type = MSG_DEPOSIT;
                printf("User to credit: "); scanf("%s", msg.target_username);
                printf("Amount: "); scanf("%d", &msg.amount);
                break;

            case 4: // Debit
                msg.type = MSG_WITHDRAW;
                printf("User to debit: "); scanf("%s", msg.target_username);
                printf("Amount: "); scanf("%d", &msg.amount);
                break;

            case 5: // Balance
                msg.type = MSG_BALANCE;
                printf("User to check: "); scanf("%s", msg.target_username);
                break;
        }

        // USE YOUR HELPER FUNCTION HERE
        send_request(msg); 
    }
}

void police_menu(char *username) {
    int choice;
    Message msg = {0};
    
    // Set identity
    strcpy(msg.username, username);
    msg.role = 'P'; 

    while (1) {
        printf("\n--- POLICE INTERFACE ---\n");
        printf("1. Check User Balance\n");
        printf("2. View User Mini Statement\n");
        printf("3. Logout\n");
        printf("Choice: ");
        scanf("%d", &choice);

        if (choice == 3) break;

        // Clean slate for new request
        memset(&msg, 0, sizeof(msg));
        strcpy(msg.username, username);
        msg.role = 'P'; // "This request is coming from the Police"

        switch(choice) {
            case 1: // Balance
                msg.type = MSG_BALANCE;
                printf("Enter Customer Username to inspect: ");
                scanf("%s", msg.target_username);
                break;

            case 2: // Statement
                msg.type = MSG_MINI_STATEMENT;
                printf("Enter Customer Username to inspect: ");
                scanf("%s", msg.target_username);
                break;

            default:
                printf("Invalid choice.\n");
                continue; // Skip the send_request part
        }

        // Send to server
        send_request(msg);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) { printf("Usage: %s <IP> <Port>\n", argv[0]); exit(1); }

    struct sockaddr_in addr = {0};
    sock = socket(AF_INET, SOCK_STREAM, 0);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[2]));
    inet_pton(AF_INET, argv[1], &addr.sin_addr);  // Converting the IP string

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Connect Error"); exit(1);
    }

    int choice;
    while(1) {
        printf("\n=== BANK SYSTEM ===\n1. Register (New)\n2. Login (Existing)\n3. Exit\nChoice: ");
        scanf("%d", &choice);
        if (choice == 3) break;

        Message msg = {0}, response;
        if (choice == 1) {
            msg.type = MSG_REGISTER;
            printf("Role (C/A/P): "); scanf(" %c", &msg.role);
            printf("Username: "); scanf("%s", msg.username);
            printf("Password: "); scanf("%s", msg.password);
            if(msg.role == 'C') { printf("Initial Deposit: "); scanf("%d", &msg.amount); }
            send_request(msg);
        } else if (choice == 2) {
            msg.type = MSG_LOGIN;
            printf("Username: "); scanf("%s", msg.username);
            printf("Password: "); scanf("%s", msg.password);
            
            send(sock, &msg, sizeof(msg), 0);
            recv(sock, &response, sizeof(response), 0);
            
            if (response.type == MSG_SUCCESS) {
                printf("Login Success! Role: %c\n", response.role);
                if (response.role == 'C') customer_menu(msg.username);
                else if (response.role == 'A') admin_menu(msg.username);
                else if (response.role == 'P') police_menu(msg.username);
            } else {
                printf("Error: %s\n", response.data);
            }
        }
    }
    close(sock);
    return 0;
}