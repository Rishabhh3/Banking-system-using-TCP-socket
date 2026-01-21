#include <stdio.h>
#include <stdlib.h>
#include "../common/logger.h"  

void add_user_to_db(char *username, char *password, char role) {
    FILE *fp = fopen("database/login.txt", "a");
    if (fp == NULL) {
        perror("Error opening login file");
        return;
    }
    fprintf(fp, "%s %s %c\n", username, password, role);
    fclose(fp);

    // --- LOG IT ---
    char log_msg[100];
    sprintf(log_msg, "New user registered: %s (Role: %c)", username, role);
    write_log(log_msg);
}

void create_customer_file(char *username) {
    char filename[100];
    
    sprintf(filename, "database/customers/%s.txt", username);
    
    FILE *fp = fopen(filename, "w");
    if (fp == NULL) {
        perror("Error creating customer file (Does 'database/customers/' exist?)");
        return;
    }
    
    // Initial balance
    fprintf(fp, "Account_Opened 0 1000\n"); 
    fclose(fp);
    
    // Log success
    char log_msg[100];
    sprintf(log_msg, "Account file created for %s", username);
    write_log(log_msg);
}

int main() {
    char username[50];
    char password[50];
    char role;

    write_log("Database Setup Utility Started");

    printf("--- Add New User ---\n");
    
    // 1. Get Role
    printf("Enter User Type (C=Customer, A=Admin, P=Police): ");
    scanf(" %c", &role); // The space before %c is important to catch newlines

    // 2. Get User ID
    printf("Enter User ID (Username): ");
    scanf("%s", username);
    
    // 3. Get Password
    printf("Enter Password: ");
    scanf("%s", password);

    // Save credentials to login.txt
    add_user_to_db(username, password, role);

    // If it is a Customer, create their account file
    if (role == 'C') {
        create_customer_file(username);
        printf("\nSuccess: User '%s' added and account file created.\n", username);
    } else {
        printf("\nSuccess: User '%s' added (No account file needed for Admin/Police).\n", username);
    }

    return 0;
}