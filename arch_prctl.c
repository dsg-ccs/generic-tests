#define _GNU_SOURCE
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>

/* Preferred headers for arch_prctl constants */
#include <linux/prctl.h>
#include <asm/prctl.h>

/* Helper printing function */
static void print_result(const char *name, long r) {
    if (r == -1) {
        printf("%-28s -> retval=-1 errno=%d (%s)\n", name, errno, strerror(errno));
    } else {
        printf("%-28s -> retval=%ld\n", name, r);
    }
}

int main(void) {
    printf("arch_prctl exercise program\n");
#ifdef SYS_arch_prctl
    printf("Using syscall number SYS_arch_prctl = %d\n", SYS_arch_prctl);
#else
    printf("Warning: SYS_arch_prctl not defined; syscall may fail to compile/run.\n");
#endif

    /* For GET_* we'll provide an address to store results */
    unsigned long orig_val = 0;
    void *orig_ptr = &orig_val;
    unsigned long out_val = 0;
    void *out_ptr = &out_val;
    long r;

    /* For SET_* we'll try some plausible values */
    unsigned long test_val = 0xdeadbeefUL;

    /* ARCH_SET_GS / ARCH_GET_GS */

    int getcmd = ARCH_GET_GS;
    int setcmd = ARCH_SET_GS;
    char NAME[] = "GS";
#if defined(ARCH_GET_GS) & defined(ARCH_SET_GS)
    errno = 0;
    orig_val = 0xfeedfaceUL;
    r = syscall(SYS_arch_prctl, getcmd, orig_ptr);
    if (r == 0) {
      printf("orig  -> %s = 0x%lx\n", NAME, orig_val);
    } else {
      print_result("ARCH_GET",r);
    }
	 
    errno = 0;
    r = syscall(SYS_arch_prctl, setcmd, test_val);
    print_result("ARCH_SET", r);

    errno = 0;
    out_val = 0xfeedfaceUL;
    r = syscall(SYS_arch_prctl, getcmd, out_ptr);
    print_result("ARCH_GET", r);
    assert(out_val == test_val);
    if (r == 0) printf("check set  -> GS = 0x%lx\n", out_val);

    errno = 0;
    r = syscall(SYS_arch_prctl, setcmd, orig_val);
    print_result("ARCH_SET reset to orig", r);

    errno = 0;
    out_val = 0xfeedfaceUL;
    r = syscall(SYS_arch_prctl, getcmd, out_ptr);
    print_result("ARCH_GET", r);
    assert(out_val == orig_val);
    if (r == 0) printf("check restored  -> %s = 0x%lx\n", NAME, out_val);
#else
    printf("ARCH_SET_GS not defined on this build (skipping)\n");
#endif

    getcmd = ARCH_GET_FS;
    setcmd = ARCH_SET_FS;
    NAME[0] = 'F';
#if defined(ARCH_GET_FS) & defined(ARCH_SET_FS)
    errno = 0;
    orig_val = 0xfeedfaceUL;
    out_val = 0xfeedfaceUL;
    r = syscall(SYS_arch_prctl, getcmd, orig_ptr);
    if (r == 0) {
      printf("orig  -> %s = 0x%lx\n", NAME, orig_val);
    } else {
      print_result("ARCH_GET",r);
    }
	 
    errno = 0;
    r = syscall(SYS_arch_prctl, setcmd, test_val);
    r = syscall(SYS_arch_prctl, getcmd, out_ptr);
    assert(out_val == test_val);
    r = syscall(SYS_arch_prctl, setcmd, orig_val);
    r = syscall(SYS_arch_prctl, getcmd, out_ptr);
    assert(out_val == orig_val);
#else
    printf("ARCH_GET/SET_GS not defined on this build (skipping)\n");
#endif


    /* Try ARCH_GET/SET_CPUID (not always present) */
#if defined(ARCH_GET_CPUID) & defined(ARCH_SET_CPUID)
    errno = 0;
    /* Many kernels expect a pointer to a struct or buffer; we pass a small buffer and see what happens */
    unsigned long cpuid_buf[4] = {0,0,0,0};
    r = syscall(SYS_arch_prctl, ARCH_GET_CPUID, (void*)cpuid_buf);
    print_result("ARCH_GET_CPUID", r);
    printf("orig  -> cpuid[0..3] = %lx %lx %lx %lx\n", cpuid_buf[0], cpuid_buf[1], cpuid_buf[2], cpuid_buf[3]);

    errno = 0;
    /* If present, attempt a trivial set (likely to fail with EINVAL/EPERM) */
    unsigned long cpuid_set_buf[4] = {1,2,3,4};
    r = syscall(SYS_arch_prctl, ARCH_SET_CPUID, (void*)cpuid_set_buf);
    print_result("ARCH_SET_CPUID", r);

    unsigned long set_cpuid_buf[4] = {0,0,0,0};
    r = syscall(SYS_arch_prctl, ARCH_GET_CPUID, (void*)set_cpuid_buf);
    print_result("ARCH_GET_CPUID", r);
    printf("new  -> cpuid[0..3] = %lx %lx %lx %lx\n", set_cpuid_buf[0], set_cpuid_buf[1], set_cpuid_buf[2], set_cpuid_buf[3]);
#else
    printf("ARCH_GET/SET_CPUID not defined on this build (skipping)\n");
#endif

    /* ARCH_MAP_VDSO (present on some kernels / architectures) */
#if defined(ARCH_MAP_VDSO)
    errno = 0;
    r = syscall(SYS_arch_prctl, ARCH_MAP_VDSO, 0);
    print_result("ARCH_MAP_VDSO", r);
#else
    printf("ARCH_MAP_VDSO not defined on this build (skipping)\n");
#endif

    /* ARCH_GET_L1I? ARCH_GET_L1D? Unknown/rare macros — try a generic list guarded by #ifdef */
#ifdef ARCH_GET_L1I
    errno = 0;
    r = syscall(SYS_arch_prctl, ARCH_GET_L1I, out_ptr);
    print_result("ARCH_GET_L1I", r);
#endif

#ifdef ARCH_GET_L1D
    errno = 0;
    r = syscall(SYS_arch_prctl, ARCH_GET_L1D, out_ptr);
    print_result("ARCH_GET_L1D", r);
#endif

#ifdef ARCH_SET_L1I
    errno = 0;
    r = syscall(SYS_arch_prctl, ARCH_SET_L1I, test_val);
    print_result("ARCH_SET_L1I", r);
#endif

#ifdef ARCH_SET_L1D
    errno = 0;
    r = syscall(SYS_arch_prctl, ARCH_SET_L1D, test_val);
    print_result("ARCH_SET_L1D", r);
#endif

    /* Try any other ARCH_* macros that may be present in headers.
       This uses a macro-wrangling pattern: define TRY if the macro exists.
       The pattern below must be expanded manually for each candidate macro. */

#ifdef ARCH_GET_XCOMP_SUPP
    errno = 0;
    r = syscall(SYS_arch_prctl, ARCH_GET_XCOMP_SUPP, out_ptr);
    print_result("ARCH_GET_XCOMP_SUPP", r);
#endif

#ifdef ARCH_SET_XCOMP
    errno = 0;
    r = syscall(SYS_arch_prctl, ARCH_SET_XCOMP, test_val);
    print_result("ARCH_SET_XCOMP", r);
#endif

#ifdef ARCH_GET_XCOMP
    errno = 0;
    r = syscall(SYS_arch_prctl, ARCH_GET_XCOMP, out_ptr);
    print_result("ARCH_GET_XCOMP", r);
#endif

#ifdef ARCH_GET_XSTATE_SIZE
    errno = 0;
    r = syscall(SYS_arch_prctl, ARCH_GET_XSTATE_SIZE, out_ptr);
    print_result("ARCH_GET_XSTATE_SIZE", r);
    if (r == 0) printf("  -> xstate size = %lu\n", out_val);
#endif

#ifdef ARCH_GET_KERNEL_BP
    errno = 0;
    r = syscall(SYS_arch_prctl, ARCH_GET_KERNEL_BP, out_ptr);
    print_result("ARCH_GET_KERNEL_BP", r);
#endif

#ifdef ARCH_SET_KERNEL_BP
    errno = 0;
    r = syscall(SYS_arch_prctl, ARCH_SET_KERNEL_BP, test_val);
    print_result("ARCH_SET_KERNEL_BP", r);
#endif

    /* If none of the optional macros were found, tell user to inspect headers */
#if !defined(ARCH_SET_GS) && !defined(ARCH_GET_GS) && !defined(ARCH_SET_FS) && !defined(ARCH_GET_FS)
    printf("No common ARCH_* macros detected at compile time. Check your kernel headers (/usr/include/asm/prctl.h or /usr/include/linux/prctl.h).\n");
#endif

    printf("Finished arch_prctl probes.\n");
    return 0;
}
