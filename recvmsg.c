// recvmsg_receiver.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 12345
#define BUF_SIZE 1024

void *recv_thread(void* tid) {
    int sockfd;
    struct sockaddr_in addr;
    char buffer[BUF_SIZE];
    struct iovec iov;
    struct msghdr msg;
    struct sockaddr_in sender_addr;
    socklen_t sender_len = sizeof(sender_addr);

    // Create a UDP socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return NULL;
    }

    // Bind to a local port
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(sockfd);
        return NULL;
    }

    // Set up the iovec structure
    iov.iov_base = buffer;
    iov.iov_len = sizeof(buffer);

    // Set up the message header
    memset(&msg, 0, sizeof(msg));
    msg.msg_name = &sender_addr;
    msg.msg_namelen = sizeof(sender_addr);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    printf("Waiting for message on port %d...\n", PORT);

    // Receive the message
    ssize_t bytes_received = recvmsg(sockfd, &msg, 0);
    if (bytes_received < 0) {
        perror("recvmsg");
        close(sockfd);
        return NULL;
    }

    // Null-terminate and print the message
    buffer[bytes_received] = '\0';
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &sender_addr.sin_addr, ip, sizeof(ip));
    printf("Received message: '%s' from %s:%d  %d:%d\n",
           buffer, ip, ntohs(sender_addr.sin_port),sender_addr.sin_addr.s_addr,sender_addr.sin_port);

    close(sockfd);
    return NULL;
}


// sendmsg_sender.c
void *sender_thread(void *id) {
    int sockfd;
    struct sockaddr_in addr;
    const char *msg_text = "Hello from sendmsg!";
    struct iovec iov;
    struct msghdr msg;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    iov.iov_base = (void*)msg_text;
    iov.iov_len = strlen(msg_text);

    memset(&msg, 0, sizeof(msg));
    msg.msg_name = &addr;
    msg.msg_namelen = sizeof(addr);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    sendmsg(sockfd, &msg, 0);

    close(sockfd);
    return NULL;
}


int main() {
  pthread_t sender_tid,recv_tid;
  FILE* file = stdout;

    // Create the recv thread
    pthread_create(&recv_tid, NULL, recv_thread, NULL);

    // Give the server some time to start up
    sleep(1);

    // Create the recv thread
    pthread_create(&sender_tid, NULL, sender_thread, NULL);


    printf("Created threads\n");
    fflush(file);
    // Wait for all threads to finish
    pthread_join(sender_tid, NULL);
    pthread_join(recv_tid, NULL);

    printf("All threads done\n");
    fflush(file);
    return 0;
}
