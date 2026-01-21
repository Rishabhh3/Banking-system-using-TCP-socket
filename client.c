#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "common/protocol.h"

int sock; // Global socket for simplicity in this menu

void customer_menu(char *username) {
    int choice;
    Message msg, response;

    while (1) {
        printf("\n--- CUSTOMER MENU (%s) ---\n", username);
        printf("1. Deposit Money\n");
        printf("2. Withdraw Money\n");
        printf("3. Logout\n");
        printf("Choice: ");
        scanf("%d", &choice);

        if (choice == 3) break;

        memset(&msg, 0, sizeof(msg));
        strcpy(msg.username, username);

        if (choice == 1) {
            msg.type = MSG_DEPOSIT;
            printf("Enter Amount to Deposit: ");
            scanf("%d", &msg.amount);
        } else if (choice == 2) {
            msg.type = MSG_WITHDRAW;
            printf("Enter Amount to Withdraw: ");
            scanf("%d", &msg.amount);
        } else {
            printf("Invalid choice.\n");
            continue;
        }

        // Send Request
        send(sock, &msg, sizeof(msg), 0);

        // Get Response
        recv(sock, &response, sizeof(response), 0);
        printf("\nSERVER RESPONSE: %s\n", response.data);
    }
}

void login_user() {
    Message msg, response;
    msg.type = MSG_LOGIN;

    printf("\n--- USER LOGIN ---\n");
    printf("Username: ");
    scanf("%s", msg.username);
    printf("Password: ");
    scanf("%s", msg.password);

    send(sock, &msg, sizeof(msg), 0);
    recv(sock, &response, sizeof(response), 0);

    if (response.type == MSG_SUCCESS) {
        printf("Login Successful! (Role: %c)\n", response.role);
        if (response.role == 'C') {
            customer_menu(msg.username);
        } else {
            printf("Admin/Police menus not implemented yet.\n");
        }
    } else {
        printf("Login Failed: %s\n", response.data);
    }
}

void register_user() {
    Message msg, response;
    msg.type = MSG_REGISTER;
    msg.role = 'C'; 

    printf("\n--- NEW REGISTRATION ---\n");
    printf("Username: ");
    scanf("%s", msg.username);
    printf("Password: ");
    scanf("%s", msg.password);
    printf("Initial Deposit: ");
    scanf("%d", &msg.amount);

    send(sock, &msg, sizeof(msg), 0);
    recv(sock, &response, sizeof(response), 0);
    printf("Server: %s\n", response.data);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <IP> <Port>\n", argv[0]);
        exit(1);
    }

    struct sockaddr_in server_addr;
    sock = socket(AF_INET, SOCK_STREAM, 0);
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(argv[2]));
    inet_pton(AF_INET, argv[1], &server_addr.sin_addr);

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection Failed");
        exit(1);
    }

    int choice;
    while(1) {
        printf("\n=== BANKING SYSTEM ===\n");
        printf("1. New User (Register)\n");
        printf("2. Existing User (Login)\n");
        printf("3. Exit\n");
        printf("Choice: ");
        scanf("%d", &choice);

        if (choice == 1) register_user();
        else if (choice == 2) login_user();
        else break;
    }

    close(sock);
    return 0;
}