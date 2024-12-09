#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>

#define SHM_KEY 0x1234
#define SEM_KEY 0x5678
#define SHM_SIZE 0x1024

// Semaphore operation functions
void sem_op(int semid, int semnum, int op) {
    struct sembuf sb;
    sb.sem_num = semnum;
    sb.sem_op = op;
    sb.sem_flg = 0;
    if (semop(semid, &sb, 1) == -1) {
        perror("semop");
        exit(1);
    }
}

int main() {
    int shmid, semid;
    char *shared_memory;

    // Create shared memory
    shmid = shmget(SHM_KEY, SHM_SIZE, IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget");
        exit(1);
    }

    // Attach shared memory
    shared_memory = (char *)shmat(shmid, NULL, 0);
    if (shared_memory == (char *)-1) {
        perror("shmat");
        exit(1);
    }

    // Create semaphores
    semid = semget(SEM_KEY, 2, IPC_CREAT | 0666); // 2 semaphores: empty and full
    if (semid == -1) {
        perror("semget");
        exit(1);
    }

    // Initialize semaphores
    semctl(semid, 0, SETVAL, 1); // empty semaphore
    semctl(semid, 1, SETVAL, 0); // full semaphore

    if (fork() == 0) {
        // Producer process
        for (int i = 0; i < 5; i++) {
            sem_op(semid, 0, -1); // Wait on empty semaphore

            snprintf(shared_memory, SHM_SIZE, "Message %d from Producer", i + 1);
            printf("Producer: Wrote '%s'\n", shared_memory);

            sem_op(semid, 1, 1); // Signal full semaphore

            sleep(1); // Simulate work
        }

        exit(0);
    } else {
        // Consumer process
        for (int i = 0; i < 5; i++) {
            sem_op(semid, 1, -1); // Wait on full semaphore

            printf("Consumer: Read '%s'\n", shared_memory);

            sem_op(semid, 0, 1); // Signal empty semaphore

            sleep(2); // Simulate work
        }

        wait(NULL); // Wait for producer to finish

        // Cleanup
        shmdt(shared_memory);
        shmctl(shmid, IPC_RMID, NULL); // Remove shared memory
        semctl(semid, 0, IPC_RMID);   // Remove semaphores

        printf("Resources cleaned up. Exiting.\n");
    }

    return 0;
}
