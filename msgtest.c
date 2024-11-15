#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>

// Define message structure
struct message {
    long msg_type;           // Message type, must be > 0
    char msg_text[100];      // Message text
};

int main() {
    key_t key;
    int msgid;
    int szrecvd;
    FILE* file = stdout;

    // Generate a unique key for the message queue  - must use an existing file name
    key = ftok("msgtest-stat", 17);  // arguments are arbitrary and meant to create unique key

    // Create the message queue and return an identifier
    msgid = msgget(key, 0666 | IPC_CREAT | IPC_EXCL);  // 0666 sets permissions for read and write

    if (msgid == -1) {
        perror("msgget failed");
        exit(1);
    }

    // Destroy the message queue
    if (msgctl(msgid, IPC_RMID, NULL) == -1) {
        perror("msgctl destroy failed");
        exit(1);
    }
    printf("Created and destroyed msg queue %d for key 0x%x\n",  msgid, key);
    fflush(file);


    // Create the message queue and return an identifier
    msgid = msgget(key, 0666 | IPC_CREAT);  // 0666 sets permissions for read and write

    if (msgid == -1) {
        perror("msgget failed");
        exit(1);
    }
    printf("Recreated msg queue %d for key 0x%x\n",  msgid, key);
    fflush(file);

    
    // Prepare two messages and send as type 1 and type 2
    struct message msg;
    msg.msg_type = 1;  // Set the message type
    strcpy(msg.msg_text, "Hello, this is a message of type 1");

    // Send the message to the message queue
    if (msgsnd(msgid, &msg, sizeof(msg.msg_text), 0) == -1) {
        perror("msgsnd failed");
        exit(1);
    }
    printf("Message 1 sent: %s\n", msg.msg_text);
    fflush(file);

    msg.msg_type = 2;  // Set the message type
    strcpy(msg.msg_text, "Hello, this is a message of type 2");

    // Send the message to the message queue
    if (msgsnd(msgid, &msg, sizeof(msg.msg_text), 0) == -1) {
        perror("msgsnd failed");
        exit(1);
    }
    printf("Message 2 sent: %s\n", msg.msg_text);
    fflush(file);

    // Receive the messages from the message queue but in reverse order using type filtering
    struct message received_msg;
    szrecvd = msgrcv(msgid, &received_msg, sizeof(received_msg.msg_text), 2, 0);
    if (szrecvd == -1) {
        perror("msgrcv failed");
        exit(1);
    }
    printf("Message 2 received: %.*s\n", szrecvd, received_msg.msg_text);
    fflush(file);

    szrecvd = msgrcv(msgid, &received_msg, sizeof(received_msg.msg_text), 1, 0);
    if (szrecvd == -1) {
        perror("msgrcv failed");
        exit(1);
    }
    printf("Message 1 received: %.*s\n", szrecvd, received_msg.msg_text);
    fflush(file);
      

    // Destroy the message queue
    if (msgctl(msgid, IPC_RMID, NULL) == -1) {
        perror("msgctl destroy failed");
        exit(1);
    }

    return 0;
}
