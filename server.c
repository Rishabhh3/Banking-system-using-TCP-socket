#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "common/protocol.h"
#include "common/logger.h"

// we send the client socket everywhere as it is the FD, that is server assigning unique no. to client,
// without it the server know what to say but to whom?
// we also have to send size because &reponse give it the address but it doesnot know it has to read till
// how many blocks in memory so size is given so that it reads only upto where the data is present and not some
// random values

// printf = print to terminal
// fprintf = print in a file in your system
// sprintf = string variable, otherwise char abc[10] = "asdsf", but i want to store it as string so use
// sprintf, it will store the string in the variable you give it

//  I cannot simply "delete" a line from a text file.
// The Strategy:
//     Open the original file (login.txt) for Reading.
//     Open a temporary file (temp.txt) for Writing.
//     Copy every line from Original to Temp EXCEPT the one we want to delete.
//     Delete Original.
//     Rename Temp to Original.

// --- HELPER: Get Balance ---
int get_current_balance(char *username) {
    char filename[100];
    sprintf(filename, "database/customers/%s.txt", username);
    FILE *fp = fopen(filename, "r");
    if (!fp) return -1; 

    int bal = 0, amt, temp_bal;// amt =trans amnt, temp = bal at current time, bal = final balance
    char line[256], trans[50]; 
    // line = temp buffer to store full line of code, trans holds the word credit or debit
    
    // Read to the last line to get current balance
    while (fgets(line, sizeof(line), fp)) { // this reads one line and stores it in line
        if (sscanf(line, "%s %d %d", trans, &amt, &temp_bal) == 3) bal = temp_bal;
        // = 3 means did you succesfully get all the features
        // it places the the 3 feautres in trans, amt, temp_bal in order and their type is mentioned
    }
    fclose(fp);
    return bal;
}

// --- HANDLER: Registration (new user) ---
void handle_register(int client_sock, Message msg) {
    Message response = {0}; // Clear memory
    
    // Check if user exists in login.txt
    FILE *fp = fopen("database/login.txt", "r"); // r = read, a = append
    char u[50], p[50], r;
    int exists = 0;
    if (fp) {
        while (fscanf(fp, "%s %s %c", u, p, &r) != EOF) { // END OF FILE
            if (strcmp(u, msg.username) == 0) exists = 1;
        }
        fclose(fp);
    }

    if (exists) {
        response.type = MSG_ERROR;
        strcpy(response.data, "Error: Username already exists.");
    } else {
        // 1. Add to login.txt
        fp = fopen("database/login.txt", "a");
        fprintf(fp, "%s %s %c\n", msg.username, msg.password, msg.role);
        fclose(fp);

        // 2. Create File (Customers only)
        if (msg.role == 'C') {
            char filename[100];
            sprintf(filename, "database/customers/%s.txt", msg.username);
            //sprintf: Writes formatted text into a string variable.
            fp = fopen(filename, "w");
            fprintf(fp, "Account_Opened %d %d\n", msg.amount, msg.amount);
            fclose(fp);
        }
        response.type = MSG_SUCCESS;
        // response.data = "abd" = ERROR
        strcpy(response.data, "Registration Successful.");
        // you cannot write name = "RIshabh", in c , you use strcpy as it reads R then store it, then it read i then store like this it happens
        write_log("New User Registered.");
    }
    send(client_sock, &response, sizeof(response), 0); // we tell the client whether it was success or failure
}

// --- HANDLER: Login ---
void handle_login(int client_sock, Message msg) {
    Message response = {0};
    FILE *fp = fopen("database/login.txt", "r");
    char u[50], p[50], r;
    int success = 0;

    if (fp) {
        while (fscanf(fp, "%s %s %c", u, p, &r) != EOF) {
            if (strcmp(u, msg.username) == 0 && strcmp(p, msg.password) == 0) {
                success = 1;
                response.role = r;
                break;
            }
        }
        fclose(fp);
    }

    if (success) {
        response.type = MSG_SUCCESS;
        strcpy(response.data, "Login Successful.");
        char log[100]; sprintf(log, "User logged in: %s", msg.username); write_log(log);
    } else {
        response.type = MSG_ERROR;
        strcpy(response.data, "Invalid Credentials.");
    }
    send(client_sock, &response, sizeof(response), 0);
    // this tell me it has to be sent to which client
}

// --- HANDLER: Balance & Mini Statement (Unified) ---
// Handles: Customer viewing own data OR Police/Admin viewing others
void handle_info(int client_sock, Message msg) {
    Message response = {0};
    
    // DECISION: Whose file do we look at?
    // If Admin/Police, use target_username. If Customer, use username.
    char *target = (msg.role == 'C') ? msg.username : msg.target_username;

    if (msg.type == MSG_BALANCE) {
        int bal = get_current_balance(target);
        if (bal == -1) strcpy(response.data, "User not found.");
        else sprintf(response.data, "Balance for %s: %d", target, bal);
        
    } else if (msg.type == MSG_MINI_STATEMENT) {
        char filename[100];
        sprintf(filename, "database/customers/%s.txt", target);// because filename = ...  is an error
        FILE *fp = fopen(filename, "r");
        if (!fp) {
            strcpy(response.data, "History not found.");
        } else {
            char line[200], buffer[1024] = "--- STATEMENT ---\n";
            // Simple: Just read and append lines , overflow can occur
            while(fgets(line, sizeof(line), fp)) {
                if(strlen(buffer) + strlen(line) < 1020) strcat(buffer, line);
            }
            strcpy(response.data, buffer);
            fclose(fp);
        }
    }
    response.type = MSG_SUCCESS;
    send(client_sock, &response, sizeof(response), 0);
}

// --- HANDLER: Transactions (Deposit/Withdraw) ---
// Handles: Customer doing own transaction OR Admin doing it for them
void handle_transaction(int client_sock, Message msg) {
    Message response = {0};
    
    // DECISION: If Admin, target the other user. If Customer, target self.
    char *target = (msg.role == 'C') ? msg.username : msg.target_username;
    
    int current_bal = get_current_balance(target);
    if (current_bal == -1) {
        strcpy(response.data, "User file not found.");
        send(client_sock, &response, sizeof(response), 0);
        return;
    }

    int new_bal = current_bal;
    char trans_type[50];

    if (msg.type == MSG_DEPOSIT) {
        new_bal += msg.amount;
        strcpy(trans_type, "Credit");
    } else if (msg.type == MSG_WITHDRAW) {
        if (current_bal < msg.amount) {
            strcpy(response.data, "Insufficient Funds.");
            send(client_sock, &response, sizeof(response), 0);
            return;
        }
        new_bal -= msg.amount;
        strcpy(trans_type, "Debit");
    }

    // Update File
    char filename[100];
    sprintf(filename, "database/customers/%s.txt", target);

    // sprintf is used because filename is in my ram, so I want to store this string in it for that i have to use
    // sprintf for file fprintf could be used and for terminal printf

    FILE *fp = fopen(filename, "a");
    fprintf(fp, "%s %d %d\n", trans_type, msg.amount, new_bal);
    fclose(fp);

    sprintf(response.data, "Success! New Balance: %d", new_bal);
    
    char log[100]; sprintf(log, "%s performed %s on %s", msg.username, trans_type, target);
    write_log(log);
    
    send(client_sock, &response, sizeof(response), 0);
}

void handle_delete_user(int client_sock, Message msg) {
    Message response = {0};
    
    // Security Check: Only Admins can delete
    if (msg.role != 'A') {
        strcpy(response.data, "Error: Unauthorized.");
        send(client_sock, &response, sizeof(response), 0);
        return;
    }

    FILE *fp = fopen("database/login.txt", "r");
    FILE *temp = fopen("database/temp.txt", "w");
    
    char u[50], p[50], r;
    int found = 0;

    if (!fp || !temp) {
        strcpy(response.data, "System Error: Database missing.");
        // Close files if they opened
        if(fp) fclose(fp); if(temp) fclose(temp);
    } else {
        // Copy Loop
        while (fscanf(fp, "%s %s %c", u, p, &r) != EOF) {
            // If this is the target, SKIP IT (don't write to temp)
            if (strcmp(u, msg.target_username) == 0) {
                found = 1;
            } else {
                // If not target, copy to temp
                fprintf(temp, "%s %s %c\n", u, p, r);
            }
        }
        fclose(fp);
        fclose(temp);

        if (found) {
            remove("database/login.txt");       // Delete old
            rename("database/temp.txt", "database/login.txt"); // Rename new
            
            // Optional: Also delete their transaction history file
            char history_file[100];
            sprintf(history_file, "database/customers/%s.txt", msg.target_username);
            remove(history_file);

            strcpy(response.data, "User Deleted Successfully.");
            write_log("Admin deleted a user.");
        } else {
            remove("database/temp.txt"); // Delete temp if we didn't use it
            strcpy(response.data, "Error: User not found.");
        }
    }
    send(client_sock, &response, sizeof(response), 0);
}

void process_client(int client_sock) {
    Message msg;    // empty container, will be filled with data sent by client

    while (recv(client_sock, &msg, sizeof(msg), 0) > 0) {   // this command pause the connection and wait for client to send data
       // when data arrives it filles the msg container, >0 means success, we have recieved some data
        printf("[SERVER] Received Request Type: %d from User: %s\n", msg.type, msg.username);
        switch (msg.type) {
            case MSG_REGISTER: handle_register(client_sock, msg); break;
            case MSG_LOGIN:    handle_login(client_sock, msg); break;
            case MSG_DEPOSIT:   // see how switch works, the next one will be executed called FallThrough
            case MSG_WITHDRAW: handle_transaction(client_sock, msg); break;
            case MSG_BALANCE:
            case MSG_MINI_STATEMENT: handle_info(client_sock, msg); break;
            case MSG_DELETE_USER: handle_delete_user(client_sock, msg); break;
        }
    }
    close(client_sock);
}

int main(int argc, char *argv[]) {
    // When you type ./server 8080 the OS breaks it into parts, argc contains how many items = 2, and
    // argv contains argv[0] = server, argv[1] = 8080

    if (argc != 2) exit(1);
    int port = atoi(argv[1]);
    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = {0};  // create struct and set every byte to zero
    // It tells the OS to reserve some memory for IPv4 addresses
    
    addr.sin_family = AF_INET;  // it is address type, like here I am using IPv4
    addr.sin_port = htons(port);    // this is to convert everything to BigEndian as 
    addr.sin_addr.s_addr = INADDR_ANY;  // INADDR_ANY means it works on both wifi or ethernet

    bind(server_sock, (struct sockaddr*)&addr, sizeof(addr));
    listen(server_sock, 5);
    write_log("Server Started");
    printf("Server listening on %d...\n", port);


    // This loop allows server to handle multiple clients at one, without this only one client could access
    // and others have to wait for him to finish
    while (1) {
        int client_sock = accept(server_sock, NULL, NULL); // accepting the connection, NULL means idc about IP just give me connection
        if (fork() == 0) {// for child
            // because of this you suddenly have 2 programs running from same line 
            // a copy of server is created just for this user
            close(server_sock);     // i dont need to listen for new clients
            process_client(client_sock);    // serve this user until they finish    
            exit(0);    // dies
        }
        close(client_sock); // back to the parent, If I dont close it then my server will run out of FD after some
        // time and it will crash
        
    }
    return 0;
}