#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <linux/futex.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <stdatomic.h>
#include <stddef.h>
FILE* file;

// Futex word aligned as required
struct robust_futex {
    int futex;
    struct robust_list list;
};

// Robust list head
struct robust_list_head robust_head = {
    .list = NULL, // Unused sentinel
    .futex_offset = offsetof(struct robust_futex, futex),
};

// Each thread registers this list
static inline long set_robust(struct robust_list_head *head) {
    return syscall(SYS_set_robust_list, head, sizeof(*head));
}

static inline long get_robust(struct robust_list_head **head, size_t *len) {
    return syscall(SYS_get_robust_list, 0, head, len);
}

static inline int futex_wait(int *addr, int val) {
    return syscall(SYS_futex, addr, FUTEX_WAIT, val, NULL, NULL, 0);
}

static inline int futex_wake(int *addr, int n) {
    return syscall(SYS_futex, addr, FUTEX_WAKE, n, NULL, NULL, 0);
}

// Global lock
static struct robust_futex lock = { .futex = 0 };

void lock_acquire() {
    int expected = 0;
    pthread_t tid = pthread_self();
    // Try to acquire lock
    while (!atomic_compare_exchange_weak(&lock.futex, &expected, (int)tid)) {
        expected = 0;
        futex_wait(&lock.futex, expected);
    }
    // Add to robust list
    lock.list.next = &robust_head.list;
    robust_head.list.next = &lock.list;
}

void lock_release() {
    atomic_store(&lock.futex, 0);
    futex_wake(&lock.futex, 1);
}

void *thread1(void *arg) {
    printf("[t1] Registering robust list and acquiring lock\n");
    fflush(file);
    set_robust(&robust_head);

    lock_acquire();
    printf("[t1] Lock acquired, now simulating crash...\n");
    fflush(file);
    sleep(1);
    printf("[t1] Exiting without releasing lock!\n");
    fflush(file);
    pthread_exit(NULL);
}

void *thread2(void *arg) {
    sleep(10); // let t1 die first
    printf("[t2] Trying to acquire lock after t1 died\n");
    fflush(file);
    int val = atomic_load(&lock.futex);

    if (val & FUTEX_OWNER_DIED) {
        printf("[t2] Detected owner died (FUTEX_OWNER_DIED). Cleaning up.\n");
	fflush(file);
        atomic_store(&lock.futex, 0);
    }

    lock_acquire();
    printf("[t2] Successfully recovered and acquired lock.\n");
    fflush(file);
    lock_release();
    printf("[t2] lock released\n");
    fflush(file);
    return NULL;
}

int main() {
    pthread_t t1, t2;
    FILE* file = stdout;
    pthread_create(&t1, NULL, thread1, NULL);
    pthread_join(t1, NULL);
    pthread_create(&t2, NULL, thread2, NULL);
    pthread_join(t2, NULL);
    return 0;
}
