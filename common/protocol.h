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

typedef struct {
    int type;           
    char username[50];
    char password[50];
    char role;          
    int amount;         
    char data[1024];    
} Message;

#endif