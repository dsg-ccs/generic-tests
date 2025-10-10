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
#define NUMBUFS 2
FILE* file;

void *recv_thread(void* tid) {
    int i;
    int sockfd;
    struct sockaddr_in addr;
    char buffer[NUMBUFS][BUF_SIZE];
    struct iovec iov[NUMBUFS];
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
    for (i=0; i<NUMBUFS; i++) {
      iov[i].iov_base = buffer[i];
      iov[i].iov_len = sizeof(buffer[i])-1;
    }

    // Set up the message header
    memset(&msg, 0, sizeof(msg));
    msg.msg_name = &sender_addr;
    msg.msg_namelen = sizeof(sender_addr);
    msg.msg_iov = iov;
    msg.msg_iovlen = NUMBUFS;

    printf("Waiting for message on port %d...\n", PORT);
    // Note that sizeof can be long or not in various arches
    printf("msg struct %p, name field %p, name len field %p, iov field %p, struct size %x\n",
	   &msg,&msg.msg_name,&msg.msg_namelen,&msg.msg_iov,(int)sizeof(msg));
    fflush(file);

    // Receive the message
    ssize_t bytes_received = recvmsg(sockfd, &msg, 0);
    if (bytes_received < 0) {
        perror("recvmsg");
        close(sockfd);
        return NULL;
    }

    // print the message using its length
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &sender_addr.sin_addr, ip, sizeof(ip));
    printf("Received message: %p for %d bytes from %s:%d  %d:%d:\n",
           (char *) msg.msg_name, msg.msg_namelen, ip, ntohs(sender_addr.sin_port),sender_addr.sin_addr.s_addr,sender_addr.sin_port);
    fflush(file);
    for (i=0; i<NUMBUFS; i++) {
      printf(" '%.*s'\n",  (int) iov[i].iov_len,  (char *) iov[i].iov_base);
      fflush(file);
    }



    close(sockfd);
    return NULL;
}


// sendmsg_sender.c
void *sender_thread(void *id) {
    int sockfd;
    struct sockaddr_in addr;
    const char *msg_text = "Hello from sendmsg!";
    const char *msg_text2 = "A little something extra";
    struct iovec iov[2];
    struct msghdr msg;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    iov[0].iov_base = (void*)msg_text;
    iov[0].iov_len = strlen(msg_text);
    iov[1].iov_base = (void*)msg_text2;
    iov[1].iov_len = strlen(msg_text2);

    memset(&msg, 0, sizeof(msg));
    msg.msg_name = &addr;
    msg.msg_namelen = sizeof(addr);
    msg.msg_iov = iov;
    msg.msg_iovlen = 2;

    char ip[INET_ADDRSTRLEN];
    printf("Sending message: '%.*s' from 127.0.0.1:%d :\n",
           msg.msg_namelen, (char *) msg.msg_name, PORT);
    printf(" '%s'\n",msg_text);
    printf(" '%s'\n",msg_text2);
    fflush(file);
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
