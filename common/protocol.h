#ifndef PROTOCOL_H
#define PROTOCOL_H

// Message Types
#define MSG_REGISTER 1
#define MSG_LOGIN    2
#define MSG_BALANCE  3
#define MSG_DEPOSIT  4
#define MSG_WITHDRAW 5
#define MSG_MINI_STATEMENT 6
#define MSG_ERROR    -1
#define MSG_SUCCESS  100
#define MSG_DELETE_USER 101
#define MSG_ADMIN_REGISTER 0

typedef struct {
    int type;           
    char username[50];       // Sender (e.g., admin1 or user1)
    char password[50];
    char role;               // 'C', 'A', 'P'
    int amount;         
    char data[1024];         // Large buffer for Mini Statements
    char target_username[50]; // NEW: For Admin to target specific users

} Message;

#endif