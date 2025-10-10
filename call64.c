/* test64.c
 *
 * Test program to exercise many Linux APIs whose names end with "64".
 *
 * Compile:
 *   gcc -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -O2 -Wall test64.c -o test64
 *
 * Run:
 *   ./test64
 *
 * Notes:
 * - Some systems expose these symbols via feature macros; we define them above.
 * - We additionally call some syscalls via syscall(SYS_...) where there's
 *   no standard wrapper (e.g. getdents64).
 */

#define _GNU_SOURCE
#define _LARGEFILE64_SOURCE
#define _FILE_OFFSET_BITS 64

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/sendfile.h>    /* for sendfile if available */
#include <dirent.h>
#include <stdint.h>
#include <inttypes.h>

/* getdents64 structures (kernel) */
#ifndef __linux__
#error "This test is for Linux systems."
#endif

/* linux dirent64 layout; glibc may provide linux_dirent64 via headers,
   but define a local one to be safe */
struct linux_dirent64 {
    ino64_t        d_ino;
    off64_t        d_off;
    unsigned short d_reclen;
    unsigned char  d_type;
    char           d_name[];
};

static void perror_exit(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

int main(void) {
    int fd = -1, fd2 = -1;
    const char *tmpname = "test64.tmp";
    ssize_t r;
    char buf[4096];
    const char *testdata = "Hello from pwrite64/pread64!\n";
    off64_t offset;
    struct stat64 st64;
    struct stat st; /* also show default stat (may be 64-bit depending on compile flags) */

    printf("=== test64: starting\n");

    /* 1) open64 (may be a macro to open) */
    fd = open64(tmpname, O_CREAT | O_RDWR | O_TRUNC, 0600);
    if (fd < 0) perror_exit("open64");

    printf("open64 -> fd=%d\n", fd);

    /* 2) pwrite64 */
    off64_t write_off = (off64_t)1234567890123LL; /* large offset to force 64-bit path */
    ssize_t wrote = pwrite64(fd, testdata, strlen(testdata), write_off);
    if (wrote < 0) {
        perror("pwrite64 (may not be available as wrapper) - trying syscall fallback");
        /* fallback to syscall(SYS_pwrite64, ...) if available */
#ifdef SYS_pwrite64
        wrote = syscall(SYS_pwrite64, fd, testdata, strlen(testdata), (off64_t)write_off);
        if (wrote < 0) perror_exit("syscall(SYS_pwrite64)");
        else printf("syscall(SYS_pwrite64) wrote=%zd\n", wrote);
#else
        fprintf(stderr, "SYS_pwrite64 not available on this platform\n");
#endif
    } else {
        printf("pwrite64 wrote=%zd at offset %" PRId64 "\n", wrote, (int64_t)write_off);
    }

    /* 3) pread64 */
    memset(buf, 0, sizeof(buf));
    ssize_t readn = pread64(fd, buf, sizeof(buf)-1, write_off);
    if (readn < 0) {
        perror("pread64 - trying syscall fallback");
#ifdef SYS_pread64
        readn = syscall(SYS_pread64, fd, buf, sizeof(buf)-1, (off64_t)write_off);
        if (readn < 0) perror_exit("syscall(SYS_pread64)");
        else printf("syscall(SYS_pread64) read=%zd bytes: %s\n", readn, buf);
#else
        fprintf(stderr, "SYS_pread64 not available on this platform\n");
#endif
    } else {
        printf("pread64 read=%zd bytes: %s\n", readn, buf);
    }

    /* 4) lseek64 */
    offset = lseek64(fd, 0, SEEK_END);
    if (offset < 0) perror_exit("lseek64");
    printf("lseek64 -> end offset=%" PRId64 "\n", (int64_t)offset);

    /* 5) fstat64 */
    if (fstat64(fd, &st64) < 0) {
        perror("fstat64 - attempt fallback to fstat");
        if (fstat(fd, &st) < 0) perror_exit("fstat fallback");
        else {
            printf("fstat fallback: size=%lld\n", (long long)st.st_size);
        }
    } else {
        printf("fstat64: size=%" PRId64 ", ino=%" PRIu64 "\n", (int64_t)st64.st_size, (uint64_t)st64.st_ino);
    }

    /* 6) stat64 and lstat64 */
    if (stat64(tmpname, &st64) < 0) perror("stat64 (may be unneeded if stat is 64)");
    else printf("stat64: size=%" PRId64 "\n", (int64_t)st64.st_size);

    if (lstat64(tmpname, &st64) < 0) perror("lstat64");
    else printf("lstat64: mode=%o\n", st64.st_mode);

    /* 7) ftruncate64 & truncate64 */
    if (ftruncate64(fd, (off64_t)0) < 0) perror("ftruncate64");
    else printf("ftruncate64 -> truncated to 0\n");

    if (truncate64(tmpname, (off64_t)0) < 0) perror("truncate64");
    else printf("truncate64 -> truncated to 0\n");

    /* write again to have contents */
    const char *again = "again\n";
    if (write(fd, again, strlen(again)) != (ssize_t)strlen(again)) perror_exit("write");

    /* 8) try sendfile64 if available (note: many systems use sendfile which handles 64-bit offsets) */
#ifdef SYS_sendfile64
    /* create a second fd for destination */
    fd2 = open64("test64.out", O_CREAT | O_RDWR | O_TRUNC, 0600);
    if (fd2 < 0) perror_exit("open64 test64.out");
    off64_t sf_off = 0;
    ssize_t sf = syscall(SYS_sendfile64, fd2, fd, &sf_off, (size_t)4096);
    if (sf < 0) perror("syscall(SYS_sendfile64)");
    else printf("syscall(SYS_sendfile64) -> %zd bytes\n", sf);
#else
    /* try sendfile (may support 64-bit on its own) */
#if defined(__NR_sendfile) && defined(SYS_sendfile)
    /* Attempt sendfile (may be 64-safe) */
    {
        off_t sf_off2 = 0;
        ssize_t sf2;
        lseek64(fd, 0, SEEK_SET);
        fd2 = open64("test64.out", O_CREAT | O_RDWR | O_TRUNC, 0600);
        if (fd2 >= 0) {
            sf2 = sendfile(fd2, fd, &sf_off2, 4096);
            if (sf2 < 0) perror("sendfile");
            else printf("sendfile -> %zd bytes\n", sf2);
        }
    }
#endif
#endif

    /* 9) getdents64 via syscall: list current directory entries */
    {
        int dfd = open64(".", O_RDONLY | O_DIRECTORY);
        if (dfd < 0) {
            perror("open64(.) for getdents64");
        } else {
            char dbuf[8192];
            int nread;
            /* SYS_getdents64 is the syscall number we want to invoke */
#ifdef SYS_getdents64
            nread = syscall(SYS_getdents64, dfd, dbuf, (size_t)sizeof(dbuf));
            if (nread < 0) {
                perror("syscall(SYS_getdents64)");
            } else {
                printf("getdents64 returned %d bytes\n", nread);
                int bpos = 0;
                while (bpos < nread) {
                    struct linux_dirent64 *d = (struct linux_dirent64 *)(dbuf + bpos);
                    printf("  d_name=%s, ino=%" PRIu64 ", d_off=%" PRId64 "\n",
                           d->d_name, (uint64_t)d->d_ino, (int64_t)d->d_off);
                    bpos += d->d_reclen;
                }
            }
#else
            fprintf(stderr, "SYS_getdents64 not available on this platform\n");
#endif
            close(dfd);
        }
    }

    /* 10) Attempt to call posix_fadvise64 if available (note not necessarily named with 64 in glibc) */
#ifdef HAVE_POSIX_FADVISE
    /* not using here; placeholder */
#endif

    /* 11) Demonstrate lseek64 to a big offset then write using pwrite64 again */
    off64_t bigoff = (off64_t) ( ( (off64_t)1 << 40 ) + 100 ); /* > 1 TB offset to exercise 64-bit offsets */
    if (lseek64(fd, bigoff, SEEK_SET) != (off64_t)-1) {
        if (write(fd, "X\n", 2) == 2) {
            printf("wrote 2 bytes at large offset %" PRId64 "\n", (int64_t)bigoff);
        }
    } else {
    }
}
