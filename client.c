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

void admin_menu(char *username) {
    int choice;
    while (1) {
        Message msg = {0};
        strcpy(msg.username, username);
        msg.role = 'A';

        printf("\n--- ADMIN MENU ---\n1. Credit User\n2. Debit User\n3. Logout\nChoice: ");
        scanf("%d", &choice);
        if (choice == 3) break;

        printf("Enter Customer Username: ");
        scanf("%s", msg.target_username);
        printf("Amount: ");
        scanf("%d", &msg.amount);

        msg.type = (choice == 1) ? MSG_DEPOSIT : MSG_WITHDRAW;
        send_request(msg);
    }
}

void police_menu(char *username) {
    int choice;
    while (1) {
        Message msg = {0};
        strcpy(msg.username, username);
        msg.role = 'P';

        printf("\n--- POLICE MENU ---\n1. Check User Balance\n2. Check User Statement\n3. Logout\nChoice: ");
        scanf("%d", &choice);
        if (choice == 3) break;

        printf("Enter Customer Username to Inspect: ");
        scanf("%s", msg.target_username);

        msg.type = (choice == 1) ? MSG_BALANCE : MSG_MINI_STATEMENT;
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