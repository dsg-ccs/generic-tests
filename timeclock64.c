#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <sys/timerfd.h>
#include <signal.h>
#include <sys/time.h>

/* Minimal timespec64 / itimerspec64 definitions for making _time64 syscalls.
   These may duplicate kernel/glibc types but are sufficient for raw syscall(2). */
struct timespec64 {
    long long tv_sec;   /* seconds (int64) */
    long        tv_nsec; /* nanoseconds (int) */
};

struct itimerspec64 {
    struct timespec64 it_interval;
    struct timespec64 it_value;
};

/* Helper to print errno nicely */
static void perr(const char *what, long r) {
    if (r == -1) printf("%s -> retval=-1 errno=%d (%s)\n", what, errno, strerror(errno));
    else printf("%s -> retval=%ld\n", what, r);
}


int main(void) {
    printf("time64-syscall test program\n");
    printf("sizeof(time_t)=%zu, sizeof(long)=%zu\n", sizeof(time_t), sizeof(long));

    /* Prepare some timespec64 values */
    struct timespec64 ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 0;

    struct timespec64 one_sec;
    one_sec.tv_sec = 1;
    one_sec.tv_nsec = 0;

    /* 1) clock_gettime64 */
#if defined(SYS_clock_gettime64) || defined(__NR_clock_gettime64)
#ifdef SYS_clock_gettime64
    errno = 0;
    {
        long r = syscall(SYS_clock_gettime64, (long)CLOCK_REALTIME, &ts);
        printf("clock_gettime64: ");
        perr("clock_gettime64", r);
        if (r == 0) printf("  -> got sec=%lld nsec=%ld\n", (long long)ts.tv_sec, (long)ts.tv_nsec);
    }
#else
    printf("clock_gettime64 macro not present (skipping)\n");
#endif
#else
    printf("clock_gettime64: no SYS_clock_gettime64/__NR_clock_gettime64 on this build\n");
#endif

    /* 2) clock_settime64 (may require root) */
#if defined(SYS_clock_settime64) || defined(__NR_clock_settime64)
#ifdef SYS_clock_settime64
    errno = 0;
    {
        struct timespec64 setts;
        setts.tv_sec = ts.tv_sec; /* try set to current time (likely EPERM) */
        setts.tv_nsec = ts.tv_nsec;
        long r = syscall(SYS_clock_settime64, (long)CLOCK_REALTIME, &setts);
        printf("clock_settime64: ");
        perr("clock_settime64", r);
    }
#else
    printf("clock_settime64 macro not present (skipping)\n");
#endif
#else
    printf("clock_settime64: no SYS_clock_settime64/__NR_clock_settime64 on this build\n");
#endif

    /* 3) clock_getres_time64 */
#if defined(SYS_clock_getres_time64) || defined(__NR_clock_getres_time64)
#ifdef SYS_clock_getres_time64
    errno = 0;
    {
        struct timespec64 res;
        long r = syscall(SYS_clock_getres_time64, (long)CLOCK_REALTIME, &res);
        printf("clock_getres_time64: ");
        perr("clock_getres_time64", r);
        if (r == 0) printf("  -> res sec=%lld nsec=%ld\n", (long long)res.tv_sec, (long)res.tv_nsec);
    }
#else
    printf("clock_getres_time64 macro not present (skipping)\n");
#endif
#else
    printf("clock_getres_time64: no SYS_clock_getres_time64/__NR_clock_getres_time64 on this build\n");
#endif

    /* 4) clock_nanosleep_time64: sleep 1 second using the 64-bit syscall if available */
#if defined(SYS_clock_nanosleep_time64) || defined(__NR_clock_nanosleep_time64)
#ifdef SYS_clock_nanosleep_time64
    errno = 0;
    {
        struct timespec64 req, rem;
        req.tv_sec = 1; req.tv_nsec = 0;
        long r = syscall(SYS_clock_nanosleep_time64, (long)CLOCK_REALTIME, 0, &req, &rem);
        printf("clock_nanosleep_time64 (1s): ");
        perr("clock_nanosleep_time64", r);
    }
#else
    printf("clock_nanosleep_time64 macro not present (skipping)\n");
#endif
#else
    printf("clock_nanosleep_time64: no SYS_clock_nanosleep_time64/__NR_clock_nanosleep_time64 on this build\n");
#endif

    /* 5) clock_adjtime64 - requires struct timex64 and privileges; try if available but be careful */
#if defined(SYS_clock_adjtime64) || defined(__NR_clock_adjtime64)
#ifdef SYS_clock_adjtime64
    errno = 0;
    {
        /* We don't attempt a real timex64 layout here. Attempt a zero pointer call to see result (likely EFAULT). */
        long r = syscall(SYS_clock_adjtime64, (long)CLOCK_REALTIME, NULL);
        printf("clock_adjtime64: ");
        perr("clock_adjtime64", r);
    }
#else
    printf("clock_adjtime64 macro not present (skipping)\n");
#endif
#else
    printf("clock_adjtime64: no SYS_clock_adjtime64/__NR_clock_adjtime64 on this build\n");
#endif

    /* Timerfd tests: create a timerfd using the normal API, then attempt timerfd_gettime64 / timerfd_settime64 */
    int tfd = timerfd_create(CLOCK_REALTIME, 0);
    if (tfd < 0) {
        printf("timerfd_create failed: %s (skipping timerfd_*64 tests)\n", strerror(errno));
    } else {
        printf("timerfd created fd=%d\n", tfd);

        /* prepare an itimerspec64 */
#if defined(SYS_timerfd_settime64) || defined(__NR_timerfd_settime64)
#ifdef SYS_timerfd_settime64
        errno = 0;
        {
            struct itimerspec64 its;
            its.it_interval.tv_sec = 0; its.it_interval.tv_nsec = 0;
            its.it_value.tv_sec = 2; its.it_value.tv_nsec = 0; /* expires in 2 seconds */
            long r = syscall(SYS_timerfd_settime64, (long)tfd, 0, &its, NULL);
            printf("timerfd_settime64: ");
            perr("timerfd_settime64", r);
        }
#else
        printf("timerfd_settime64 macro not present (skipping)\n");
#endif
#else
        printf("timerfd_settime64: no SYS_timerfd_settime64/__NR_timerfd_settime64 on this build\n");
#endif

#if defined(SYS_timerfd_gettime64) || defined(__NR_timerfd_gettime64)
#ifdef SYS_timerfd_gettime64
        errno = 0;
        {
            struct itimerspec64 got;
            long r = syscall(SYS_timerfd_gettime64, (long)tfd, &got);
            printf("timerfd_gettime64: ");
            perr("timerfd_gettime64", r);
            if (r == 0) {
                printf("  -> next expiration in %lld sec %ld nsec\n", (long long)got.it_value.tv_sec, got.it_value.tv_nsec);
            }
        }
#else
        printf("timerfd_gettime64 macro not present (skipping)\n");
#endif
#else
        printf("timerfd_gettime64: no SYS_timerfd_gettime64/__NR_timerfd_gettime64 on this build\n");
#endif

        close(tfd);
    }

    /* POSIX timer_gettime64 / timer_settime64 using a timer created with timer_create (if available) */
    timer_t tid = 0;
    struct sigevent sev;
    memset(&sev, 0, sizeof(sev));
    if (timer_create(CLOCK_REALTIME, &sev, &tid) == 0) {
        printf("posix timer created (timer_t=%p)\n", (void*)tid);

#if defined(SYS_timer_settime64) || defined(__NR_timer_settime64)
#ifdef SYS_timer_settime64
        errno = 0;
        {
            struct itimerspec64 its;
            its.it_interval.tv_sec = 0; its.it_interval.tv_nsec = 0;
            its.it_value.tv_sec = 5; its.it_value.tv_nsec = 0;
            long r = syscall(SYS_timer_settime64, (long)tid, 0, &its, NULL);
            printf("timer_settime64: ");
            perr("timer_settime64", r);
        }
#else
        printf("timer_settime64 macro not present (skipping)\n");
#endif
#else
        printf("timer_settime64: no SYS_timer_settime64/__NR_timer_settime64 on this build\n");
#endif

#if defined(SYS_timer_gettime64) || defined(__NR_timer_gettime64)
#ifdef SYS_timer_gettime64
        errno = 0;
        {
            struct itimerspec64 got;
            long r = syscall(SYS_timer_gettime64, (long)tid, &got);
            printf("timer_gettime64: ");
            perr("timer_gettime64", r);
            if (r == 0) {
                printf("  -> timer next in %lld sec %ld nsec\n", (long long)got.it_value.tv_sec, got.it_value.tv_nsec);
            }
        }
#else
        printf("timer_gettime64 macro not present (skipping)\n");
#endif
#else
        printf("timer_gettime64: no SYS_timer_gettime64/__NR_timer_gettime64 on this build\n");
#endif

        timer_delete(tid);
    } else {
        printf("timer_create failed: %s (skipping timer_gettime64/timer_settime64)\n", strerror(errno));
    }

    printf("done.\n");
    return 0;
}
