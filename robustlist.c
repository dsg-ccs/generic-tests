// From ChatGPT - give an example of using the get_robust_list linux system call
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <linux/futex.h>
#include <linux/unistd.h>
#include <sys/types.h>
#include <stdint.h>
#include <errno.h>

/*
These are defined in futex.h but ChatGPT put them in by mistake
I left them here for reference
 
struct robust_list {
    struct robust_list *next;
};

struct robust_list_head {
    struct robust_list list;
    long futex_offset;
    struct robust_list *list_op_pending;
};
*/

int main() {
    int ret;
    pid_t tid = syscall(SYS_gettid);
    struct robust_list_head head = { .list = NULL};
    size_t head_size = sizeof(head);
    size_t len = 0;

    ret = syscall(SYS_set_robust_list, &head, head_size);
    if (ret != 0) {
        perror("set_robust_list failed");
        return 1;
    }

    struct robust_list_head *got_head = NULL;
    size_t got_size = 0;
    ret = syscall(SYS_get_robust_list, tid, &got_head, &got_size);
    if (ret == -1) {
        perror("get_robust_list");
        return 2;
    }

    printf("Robust list head: %p\n", (void *)got_head);
    printf("Length: %zu\n", got_size);
    if (got_head) {
        printf("Futex offset: %ld\n", got_head->futex_offset);
        printf("List pending: %p\n", (void *)got_head->list_op_pending);
    }

    return 0;
}
