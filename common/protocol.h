#ifndef PROTOCOL_H
#define PROTOCOL_H

// Message Types
#define MSG_REGISTER 1
#define MSG_LOGIN    2
#define MSG_BALANCE  3
#define MSG_DEPOSIT  4
#define MSG_WITHDRAW 5
#define MSG_ERROR    -1
#define MSG_SUCCESS  100

typedef struct {
    int type;           // What action to take?
    char username[50];
    char password[50];
    char role;          // 'C', 'A', 'P'
    int amount;         // For transactions/opening balance
    char data[256];     // For server responses (messages)
} Message;

#endif