#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <poll.h>
#include <sys/wait.h>

#define PORT 8080
#define NUM_THREADS 3
#define BUFFER_SIZE 1024
#define TIMEOUT 5000 // Poll timeout in milliseconds (5 seconds)

// Function to handle the client thread
void *client_thread(void *id) {
    FILE* file = stdout;
    int thread_id = *((int*)id);
    int sock = 0;
    struct sockaddr_in server_addr;
    char *message = "Hello from client";
    char buffer[BUFFER_SIZE] = {0};

    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Socket creation error\n");
        return NULL;
    }

    // Set up the server address struct
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    // Convert IPv4 address from text to binary form
    if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) {
        printf("Invalid address / Address not supported\n");
        return NULL;
    }

    // Connect to the server
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        printf("Connection failed\n");
        return NULL;
    }

    sleep(6-2*thread_id);
    // Send a message to the server
    sprintf(buffer,"%s %d",message,thread_id);
    send(sock, buffer, strlen(buffer), 0);
    printf("Client %d: Sent message: %s\n", thread_id, buffer);
    fflush(file);

    // Receive a message from the server
    read(sock, buffer, BUFFER_SIZE);
    printf("Client %d: Received message: %s\n", thread_id, buffer);
    fflush(file);

    close(sock);
    printf("Client %d: Closed socket\n",thread_id);
    fflush(file);
    return NULL;
}

void closefd(int i, int fd) {
  if (close(fd) == -1) {
    perror("Error closing fd");
    printf("Error closing fd %d %d\n",i,fd);
  } else {
    printf("Closed fd %d %d\n",i,fd);
  }
}

// Function to handle the server-side thread (parent)
void *server_thread() {
    int server_fd, valread;
    int client_sockets[NUM_THREADS] = {0};
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};
    char *response = "Hello from server";
    FILE* file = stdout;
    struct pollfd fds[NUM_THREADS];
    int num_fds = 0;
    int num_open = 0;
    int timeouts = 0;

    // Create the server socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    // Bind the socket to the port
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server: Listening on port %d...\n", PORT);
    fflush(file);

    // Accept and handle incoming connections from the clients
    for (int i = 0; i < NUM_THREADS; i++) {
        if ((client_sockets[i] = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
            perror("Accept failed");
            exit(EXIT_FAILURE);
        }
	fds[i].fd = client_sockets[i];
	fds[i].events = POLLIN;
	num_fds++;
    }
    printf("Server: Accepted connections from all children\n");
    fflush(file);

    // Poll loop to check for incoming messages
    num_open = num_fds;
    while ((num_open > 0) || (timeouts > 3)) {

        // Poll the sockets for any activity
        int poll_count = poll(fds, num_fds, TIMEOUT);
	//printf("Server: Polling found %d\n",poll_count);
      	fflush(file);

        if (poll_count < 0) {
            perror("Poll error");
            exit(EXIT_FAILURE);
        } else if (poll_count == 0) {
            printf("Server: Poll timed out. No data in %d ms\n", TIMEOUT);
	    fflush(file);
	    timeouts++;
        } else {
            for (int i = 0; i < num_fds; i++) {
                if (fds[i].revents & POLLIN) {
                    // Read the incoming message
                    int valread = read(fds[i].fd, buffer, BUFFER_SIZE);
                    if (valread > 0) {
                        printf("Server: Received from client %d: %s\n", i + 1, buffer);
			fflush(file);
                        // Send a response back to the client
                        char *response = "Hello from server";
                        send(fds[i].fd, response, strlen(response), 0);
                    } else if (valread == 0) {
                        printf("Server: Received EOF from client %d\n", i + 1);
			closefd(i, fds[i].fd);
			fflush(file);
			num_open--;
		    }
                } else if (fds[i].revents != 0) {  //is POLLERR or POLLHUP
		  // This does not seem to stop poll from finding these over and over so skip print
		  //	printf("Server: Polled 0x%x from client %d\n", fds[i].revents, i + 1);
		  //    fflush(file);
		  fds[i].revents=0;
		  fds[i].events=0;
		} 
            }
	}
	if (poll_count == 3) {
	  break;
	}
    }


    close(server_fd);
    printf("Server: Done\n");
    fflush(file);
    return NULL;
}

int main() {
    pthread_t threads[NUM_THREADS];
    int thread_ids[NUM_THREADS];
    FILE* file = stdout;
    pid_t cpid,w;
    int status;

    cpid = fork();
    if (cpid == -1) {
      perror("fork failed");
      exit(-1);
    }

    if (cpid == 0) {
      /* Child */
      // Run server thread
      server_thread();
    } else {
      /* Parent */
      // Give the server some time to start up
      sleep(1);

      // Create the client threads
      for (int i = 0; i < NUM_THREADS; i++) {
        thread_ids[i] = i + 1;
	pthread_create(&threads[i], NULL, client_thread, (void*)&thread_ids[i]);
      }

      printf("Created %d threads\n",NUM_THREADS);
      fflush(file);
      // Wait for all threads to finish
      for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
      }
      printf("All %d child threads done\n",NUM_THREADS);
      fflush(file);

      w = waitpid(cpid,&status, WUNTRACED | WCONTINUED);
      if (w == -1) {
	perror("Waitpid failed");
	exit(-1);
      }
      if (WIFEXITED(status)) {
	printf("server exited, status=%d\n", WEXITSTATUS(status));
      } else if (WIFSIGNALED(status)) {
	printf("server killed by signal %d\n", WTERMSIG(status));
      } else if (WIFSTOPPED(status)) {
	printf("server stopped by signal %d\n", WSTOPSIG(status));
      } else if (WIFCONTINUED(status)) {
	printf("server continued\n");
      }
      printf("Server fork done\n");
      fflush(file);

    }
    return 0;
}
 
