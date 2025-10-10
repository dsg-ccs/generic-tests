// iovec_demo.c
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/uio.h>       // struct iovec, readv/writev, process_vm_{read,write}v declarations (maybe)
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/syscall.h>   // SYS_process_vm_readv etc.
#include <linux/unistd.h>
#include <stdint.h>
#include <time.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#ifndef SYS_process_vm_readv
# if defined(__x86_64__)
#  define SYS_process_vm_readv 310
#  define SYS_process_vm_writev 311
# else
#  error "Unknown arch for process_vm_readv syscall number; update code or include correct header."
# endif
#endif

// This global just to allow easy flushing of stdin which makes checking of output easier
FILE* file;

static void die(const char *op) {
    perror(op);
    exit(EXIT_FAILURE);
}

/* Simple helper to print a buffer as string (safe) */
static void show_buf(const char *tag, const char *buf, ssize_t n) {
    printf("%s: [%.*s]\n", tag, (int)n, buf);
    fflush(file);
}

/* Demonstrate writev/readv using a pipe */
static void demo_readv_writev(void) {
    printf("=== demo_readv_writev ===\n");
    fflush(file);
    int p[2];
    if (pipe(p) != 0) die("pipe");

    const char *a = "Hello, ";
    const char *b = "writev/readv!";
    struct iovec wv[2] = {
        { .iov_base = (void*)a, .iov_len = strlen(a) },
        { .iov_base = (void*)b, .iov_len = strlen(b) }
    };

    printf("Writev vec at %p, with msgs at %p and %p\n",wv,a,b);
    fflush(file);
    ssize_t wn = writev(p[1], wv, 2);
    if (wn < 0) die("writev");

    char buf[64];
    struct iovec rv[2] = {
        { .iov_base = buf, .iov_len = 7 },           // "Hello, "
        { .iov_base = buf + 7, .iov_len = sizeof(buf)-7 }
    };
    printf("Readv vec at %p, with buf at  %p\n",rv,buf);
    fflush(file);
    ssize_t rn = readv(p[0], rv, 2);
    if (rn < 0) die("readv");
    show_buf("readv got", buf, rn);

    close(p[0]); close(p[1]);
    putchar('\n');
}

/* Demonstrate preadv/pwritev to/from a temp file */
static void demo_preadv_pwritev(void) {
    printf("=== demo_preadv_pwritev ===\n");
    fflush(file);
    char tmpname[] = "/tmp/iovec_demoXXXXXX";
    int fd = mkstemp(tmpname);
    if (fd < 0) die("mkstemp");
    unlink(tmpname); // remove filename; file remains open

    const char *s1 = "Segment-1:";
    const char *s2 = "Segment-2:";
    struct iovec wv[2] = {
        { .iov_base = (void*)s1, .iov_len = strlen(s1) },
        { .iov_base = (void*)s2, .iov_len = strlen(s2) }
    };
    // pwritev: write both segments at offset 0
    ssize_t wn = pwritev(fd, wv, 2, 0);
    if (wn < 0) die("pwritev");

    // read back using preadv into separate buffers
    char b1[32] = {0}, b2[32] = {0};
    struct iovec rv[2] = {
        { .iov_base = b1, .iov_len = sizeof(b1)-1 },
        { .iov_base = b2, .iov_len = sizeof(b2)-1 }
    };
    ssize_t rn = preadv(fd, rv, 2, 0);
    if (rn < 0) die("preadv");
    printf("preadv read total %zd bytes\n", rn);
    fflush(file);
    show_buf("buf1", b1, strlen(b1));
    show_buf("buf2", b2, strlen(b2));
    close(fd);
    putchar('\n');
}

#define PORT 12345
/* Demonstrate sendmsg/recvmsg over a UNIX datagram socketpair */
static void demo_sendmsg_recvmsg(void) {
    struct sockaddr_un addr;
    struct sockaddr_un sender_addr;
    socklen_t sender_len = sizeof(addr);
    printf("=== demo_sendmsg_recvmsg ===\n");
    fflush(file);
    int sv[2];
    if (socketpair(AF_UNIX, SOCK_DGRAM, 0, sv) != 0) die("socketpair");

    const char *p1 = "Part-A-";
    const char *p2 = "Part-B";
    struct iovec iov[2] = {
        { .iov_base = (void*)p1, .iov_len = strlen(p1) },
        { .iov_base = (void*)p2, .iov_len = strlen(p2) },
    };
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    // Using socket pair so address is anonymous (ie. empty
    strncpy(addr.sun_path, "\0", sizeof(addr.sun_path - 1));

    struct msghdr msg = {
      //        .msg_name = &addr,
      //  .msg_namelen = sizeof(addr),
        .msg_iov = iov,
        .msg_iovlen = 2,
        .msg_control = NULL,
        .msg_controllen = 0,
        .msg_flags = 0
    };

    printf("Sending message: with not explicit name since using socket pair :\n");

    printf("vectors at %p '%s' '%s' \n",iov,p1,p2);
    fflush(file);
    ssize_t s = sendmsg(sv[0], &msg, 0);
    if (s < 0) die("sendmsg");

    char rbuf[64];
    struct iovec riov[2] = {{.iov_base = rbuf, .iov_len = sizeof(rbuf)/2},
                            {.iov_base = rbuf+sizeof(rbuf)/2, .iov_len = sizeof(rbuf)/2}};

    struct msghdr rmsg = {
        .msg_name = &sender_addr,
        .msg_namelen = sizeof(sender_addr),
        .msg_iov = riov,
        .msg_iovlen = 2,
    };

    ssize_t r = recvmsg(sv[1], &rmsg, 0);
    if (r < 0) die("recvmsg");
    printf("Read message named: '%.*s'\n  text: '%.*s' vectors at %p \n",
           msg.msg_namelen, (char *) msg.msg_name, (int)r, rbuf, riov);
    fflush(file);

    close(sv[0]); close(sv[1]);
    putchar('\n');
}

/* Demonstrate sendmmsg/recvmmsg (batch sending/receiving) */
static void demo_sendmmsg_recvmmsg(void) {
    printf("=== demo_sendmmsg_recvmmsg ===\n");
    fflush(file);
    int sv[2];
    if (socketpair(AF_UNIX, SOCK_DGRAM, 0, sv) != 0) die("socketpair");

    // prepare two messages to send in one syscall
    const char *msgs[2][2] = {
        { "msg1-part1-", "msg1-part2" },
        { "msg2-part1-", "msg2-part2" }
    };

    struct mmsghdr mmsgs[2];
    struct iovec iovecs[2][2];
    memset(mmsgs, 0, sizeof(mmsgs));
    for (int m = 0; m < 2; ++m) {
        iovecs[m][0].iov_base = (void*)msgs[m][0];
        iovecs[m][0].iov_len  = strlen(msgs[m][0]);
        iovecs[m][1].iov_base = (void*)msgs[m][1];
        iovecs[m][1].iov_len  = strlen(msgs[m][1]);

        mmsgs[m].msg_hdr.msg_iov = iovecs[m];
        mmsgs[m].msg_hdr.msg_iovlen = 2;
    }

    int sent = sendmmsg(sv[0], mmsgs, 2, 0);
    if (sent < 0) die("sendmmsg");
    printf("sendmmsg sent %d messages\n", sent);
    fflush(file);

    // receive two messages
    struct mmsghdr rmsgs[2];
    struct iovec riov[2];
    char rbufs[2][64];
    memset(rmsgs, 0, sizeof(rmsgs));
    for (int i = 0; i < 2; ++i) {
        riov[i].iov_base = rbufs[i];
        riov[i].iov_len = sizeof(rbufs[i]);
        rmsgs[i].msg_hdr.msg_iov = &riov[i];
        rmsgs[i].msg_hdr.msg_iovlen = 1;
    }

    int recvd = recvmmsg(sv[1], rmsgs, 2, 0, NULL);
    if (recvd < 0) die("recvmmsg");
    printf("recvmmsg received %d messages\n", recvd);
    fflush(file);
    for (int i = 0; i < recvd; ++i)
        show_buf("batch recv", rbufs[i], rmsgs[i].msg_len);

    close(sv[0]); close(sv[1]);
    putchar('\n');
}

/* Demonstrate vmsplice: splice memory pages into a pipe (user-space iovec) */
static void demo_vmsplice(void) {
    printf("=== demo_vmsplice ===\n");
    fflush(file);
#ifdef SYS_vmsplice
    int p[2];
    if (pipe(p) != 0) die("pipe");

    char data[] = "Data-for-vmsplice";
    struct iovec iov = { .iov_base = data, .iov_len = sizeof(data)-1 };

    // vmsplice: splice from user iovec into pipe
    ssize_t n = syscall(SYS_vmsplice, p[1], &iov, 1, 0);
    if (n < 0) die("vmsplice");

    char buf[64];
    ssize_t r = read(p[0], buf, sizeof(buf));
    if (r < 0) die("read after vmsplice");
    show_buf("vmsplice->pipe read", buf, r);

    close(p[0]); close(p[1]);
#else
    printf("vmsplice not available on this platform/build.\n");
    fflush(file);
#endif
    putchar('\n');
}

/* Demonstrate process_vm_readv/writev by reading/writing our own memory */
static void demo_process_vm_readv_writev(void) {
    printf("=== demo_process_vm_readv_writev (self) ===\n");
    fflush(file);
    // target address: a local buffer
    char secret[] = "SECRET_DATA_12345";
    char out[64] = {0};

    struct iovec local[1];
    struct iovec remote[1];

    // remote points to 'secret' in our own address space
    remote[0].iov_base = secret;
    remote[0].iov_len  = sizeof(secret);

    // local buffer to receive into
    local[0].iov_base  = out;
    local[0].iov_len   = sizeof(out);

    pid_t pid = getpid();

    // read our own memory via process_vm_readv
    long n = syscall(SYS_process_vm_readv, pid, local, 1, remote, 1, 0);
    if (n < 0) {
        // Some kernels or sandboxes prevent this; warn but continue
        fprintf(stderr, "process_vm_readv failed: %s\n", strerror(errno));
    } else {
        printf("process_vm_readv read %ld bytes: ", n);
	fflush(file);
        show_buf("out", out, (int)n);
    }

    // now try process_vm_writev to overwrite a buffer (write into our 'out' buffer)
    const char newdata[] = "WRITTEN_BY_process_vm_writev";
    struct iovec src = { .iov_base = (void*)newdata, .iov_len = sizeof(newdata)-1 };
    struct iovec dst = { .iov_base = out, .iov_len = sizeof(out) };
    long w = syscall(SYS_process_vm_writev, pid, &src, 1, &dst, 1, 0);
    if (w < 0) {
        fprintf(stderr, "process_vm_writev failed: %s\n", strerror(errno));
    } else {
        printf("process_vm_writev wrote %ld bytes; out now: ", w);
	fflush(file);
        show_buf("out", out, (int)strlen(out));
    }
    putchar('\n');
}

int main(void) {
  FILE* file = stdout;
    printf("iovec/syscall demo\n\n");
    fflush(file);
    demo_readv_writev();
    demo_preadv_pwritev();
    demo_sendmsg_recvmsg();
    demo_sendmmsg_recvmmsg();
    demo_vmsplice();
    demo_process_vm_readv_writev();

    printf("Done.\n");
    fflush(file);
    return 0;
}
