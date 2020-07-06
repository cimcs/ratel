/* **********************************************************
 * Copyright (c) 2018-2020 Ratel Authors.  All rights reserved.
 * **********************************************************/

/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * * Neither the name of the copyright holder nor the names of its contributors may be
 *   used to endorse or promote products derived from this software without
 *   specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL VMWARE, INC. OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include <unistd.h>
#include <fcntl.h>
#include <pwd.h>

#include <sys/types.h>
#include <sys/syscall.h>

#include "sgx_urts.h"
#include "App.h"
#include "dynamorio_u.h"

#include "thread_helper.h"
#include <sys/mman.h>
#include <sys/socket.h>
#include "iovec_helper.h"

/* -------------------------Begin: define SYSCALL stub -------------------------*/
void syscall_stub(void)
{
    __asm__ volatile(
        "local_syscall:\n\t"
        "mov    %rdi,%rax\n\t"
        "mov    %rsi,%rdi\n\t"
        "mov    %rdx,%rsi\n\t"
        "mov    %rcx,%rdx\n\t"
        "mov    %r8,%r10\n\t"
        "mov    %r9,%r8\n\t"
        "mov    0x8(%rsp),%r9\n\t"
        "syscall \n\t"
        "retq");
}
long local_syscall();

#define syscall local_syscall
/* -------------------------End: define SYSCALL stub -------------------------*/

//Please make me beautiful
void ocall_cpuid_ToNN(void *T, int l, int eax, int ecx)
{
    int a, b, c, d;

    asm volatile(
        "\tmov %4, %%eax\n"
        "\tmov %5, %%ecx\n"
        "\tcpuid\n"
        "\tmov %%eax, %0\n"
        "\tmov %%ebx, %1\n"
        "\tmov %%ecx, %2\n"
        "\tmov %%edx, %3\n"
        : "=m"(a), "=m"(b), "=m"(c), "=m"(d)
        : "rm"(eax), "rm"(ecx));
    ((int *)T)[0] = a;
    ((int *)T)[1] = b;
    ((int *)T)[2] = c;
    ((int *)T)[3] = d;
    printf("%s\n", __FUNCTION__);
}

void ocall_rdtsc_To(void *T, int l)
{
    unsigned long long *t = T;
    unsigned hi, lo;

    __asm__ __volatile__("rdtsc"
                         : "=a"(lo), "=d"(hi));
    *t = ((unsigned long long)lo) | (((unsigned long long)hi) << 32);
}

char *syscall_name[] = {
    "read",
    "write",
    "open",
    "close",
    "stat",
    "fstat",
    "lstat",
    "poll",
    "lseek",
    "mmap",
    "mprotect",
    "munmap",
    "brk",
    "rt_sigaction",
    "rt_sigprocmask",
    "rt_sigreturn",
    "ioctl",
    "pread64",
    "pwrite64",
    "readv",
    "writev",
    "access",
    "pipe",
    "select",
    "sched_yield",
    "mremap",
    "msync",
    "mincore",
    "madvise",
    "shmget",
    "shmat",
    "shmctl",
    "dup",
    "dup2",
    "pause",
    "nanosleep",
    "getitimer",
    "alarm",
    "setitimer",
    "getpid",
    "sendfile",
    "socket",
    "connect",
    "accept",
    "sendto",
    "recvfrom",
    "sendmsg",
    "recvmsg",
    "shutdown",
    "bind",
    "listen",
    "getsockname",
    "getpeername",
    "socketpair",
    "setsockopt",
    "getsockopt",
    "clone",
    "fork",
    "vfork",
    "execve",
    "exit",
    "wait4",
    "kill",
    "uname",
    "semget",
    "semop",
    "semctl",
    "shmdt",
    "msgget",
    "msgsnd",
    "msgrcv",
    "msgctl",
    "fcntl",
    "flock",
    "fsync",
    "fdatasync",
    "truncate",
    "ftruncate",
    "getdents",
    "getcwd",
    "chdir",
    "fchdir",
    "rename",
    "mkdir",
    "rmdir",
    "creat",
    "link",
    "unlink",
    "symlink",
    "readlink",
    "chmod",
    "fchmod",
    "chown",
    "fchown",
    "lchown",
    "umask",
    "gettimeofday",
    "getrlimit",
    "getrusage",
    "sysinfo",
    "times",
    "ptrace",
    "getuid",
    "syslog",
    "getgid",
    "setuid",
    "setgid",
    "geteuid",
    "getegid",
    "setpgid",
    "getppid",
    "getpgrp",
    "setsid",
    "setreuid",
    "setregid",
    "getgroups",
    "setgroups",
    "setresuid",
    "getresuid",
    "setresgid",
    "getresgid",
    "getpgid",
    "setfsuid",
    "setfsgid",
    "getsid",
    "capget",
    "capset",
    "rt_sigpending",
    "rt_sigtimedwait",
    "rt_sigqueueinfo",
    "rt_sigsuspend",
    "sigaltstack",
    "utime",
    "mknod",
    "uselib",
    "personality",
    "ustat",
    "statfs",
    "fstatfs",
    "sysfs",
    "getpriority",
    "setpriority",
    "sched_setparam",
    "sched_getparam",
    "sched_setscheduler",
    "sched_getscheduler",
    "sched_get_priority_max",
    "sched_get_priority_min",
    "sched_rr_get_interval",
    "mlock",
    "munlock",
    "mlockall",
    "munlockall",
    "vhangup",
    "modify_ldt",
    "pivot_root",
    "_sysctl",
    "prctl",
    "arch_prctl",
    "adjtimex",
    "setrlimit",
    "chroot",
    "sync",
    "acct",
    "settimeofday",
    "mount",
    "umount2",
    "swapon",
    "swapoff",
    "reboot",
    "sethostname",
    "setdomainname",
    "iopl",
    "ioperm",
    "create_module",
    "init_module",
    "delete_module",
    "get_kernel_syms",
    "query_module",
    "quotactl",
    "nfsservctl",
    "getpmsg",
    "putpmsg",
    "afs_syscall",
    "tuxcall",
    "security",
    "gettid",
    "readahead",
    "setxattr",
    "lsetxattr",
    "fsetxattr",
    "getxattr",
    "lgetxattr",
    "fgetxattr",
    "listxattr",
    "llistxattr",
    "flistxattr",
    "removexattr",
    "lremovexattr",
    "fremovexattr",
    "tkill",
    "time",
    "futex",
    "sched_setaffinity",
    "sched_getaffinity",
    "set_thread_area",
    "io_setup",
    "io_destroy",
    "io_getevents",
    "io_submit",
    "io_cancel",
    "get_thread_area",
    "lookup_dcookie",
    "epoll_create",
    "epoll_ctl_old",
    "epoll_wait_old",
    "remap_file_pages",
    "getdents64",
    "set_tid_address",
    "restart_syscall",
    "semtimedop",
    "fadvise64",
    "timer_create",
    "timer_settime",
    "timer_gettime",
    "timer_getoverrun",
    "timer_delete",
    "clock_settime",
    "clock_gettime",
    "clock_getres",
    "clock_nanosleep",
    "exit_group",
    "epoll_wait",
    "epoll_ctl",
    "tgkill",
    "utimes",
    "vserver",
    "mbind",
    "set_mempolicy",
    "get_mempolicy",
    "mq_open",
    "mq_unlink",
    "mq_timedsend",
    "mq_timedreceive",
    "mq_notify",
    "mq_getsetattr",
    "kexec_load",
    "waitid",
    "add_key",
    "request_key",
    "keyctl",
    "ioprio_set",
    "ioprio_get",
    "inotify_init",
    "inotify_add_watch",
    "inotify_rm_watch",
    "migrate_pages",
    "openat",
    "mkdirat",
    "mknodat",
    "fchownat",
    "futimesat",
    "newfstatat",
    "unlinkat",
    "renameat",
    "linkat",
    "symlinkat",
    "readlinkat",
    "fchmodat",
    "faccessat",
    "pselect6",
    "ppoll",
    "unshare",
    "set_robust_list",
    "get_robust_list",
    "splice",
    "tee",
    "sync_file_range",
    "vmsplice",
    "move_pages",
    "utimensat",
    "epoll_pwait",
    "signalfd",
    "timerfd_create",
    "eventfd",
    "fallocate",
    "timerfd_settime",
    "timerfd_gettime",
    "accept4",
    "signalfd4",
    "eventfd2",
    "epoll_create1",
    "dup3",
    "pipe2",
    "inotify_init1",
    "preadv",
    "pwritev",
    "rt_tgsigqueueinfo",
    "perf_event_open",
    "recvmmsg",
    "fanotify_init",
    "fanotify_mark",
    "prlimit64",
    "name_to_handle_at",
    "open_by_handle_at",
    "clock_adjtime",
    "syncfs",
    "sendmmsg",
    "setns",
    "getcpu",
    "process_vm_readv",
    "process_vm_writev"};

/* These two functions are used for debugging. */
void ocall_print_str(const char *str)
{
    printf("%s\n", str);
}

void ocall_print_syscallname(long sysno)
{
    if (sysno < 0 || sysno >= sizeof(syscall_name) / sizeof(char *))
        printf("Invalid syscall No.: %d\n", (int)sysno);
    else
        printf("%s, %d\n", syscall_name[sysno], (int)sysno);
}

/* system calls */
void echo_fun_return(long sysno, bool implemented, const char *fname, long ret)
{
    if (!implemented)
    {
        printf("sysno: %-4d not implemented!\n", (int)sysno);
    }
    //fflush(stdout);
}

/*-----------------------------------syscalls with 0 parameters--------------------------*/
long ocall_syscall_0(long sysno)
{
    long ret;
    bool b = false;

    switch (sysno)
    {
    case SYS_getpid:
    case SYS_getppid:
    case SYS_getpgrp:
    case SYS_gettid:
    case SYS_geteuid:
    case SYS_getuid:
    case SYS_getegid:
    case SYS_getgid:
    case SYS_sched_yield:
    case SYS_sync:
    case SYS_restart_syscall:
        ret = syscall(sysno);
        b = true;
        break;
    }

    echo_fun_return(sysno, b, __FUNCTION__, ret);

    return ret;
}

/*-----------------------------------syscalls with 1 parameters--------------------------*/
#include <pthread.h>
volatile int g_trd_num_helper = 0;
long ocall_syscall_1_N(long sysno, long N1)
{
    long ret;
    bool b = false;

    switch (sysno)
    {
    case SYS_exit_group: /* fix-me: allow out-encalve code to do cleanup */
    case SYS_exit:
        {
            int tid = syscall(SYS_gettid);
            int pid = syscall(SYS_getpid);
            if(tid != pid)
            {
                if (SYS_exit == sysno)   
                {                    
                    int trd_num = 0;
                    long tid = syscall(SYS_gettid);
                    extern pthread_mutex_t lock_m;
                    pthread_mutex_lock(&lock_m);
                    extern volatile int g_trd_num;
                    extern sgx_thread_priv_params *trd_priv_params;

                    while (trd_num < g_trd_num) 
                    {
                        /* the argument clone_child_stack as an index to find out which td_hctx[x] belongs to ECALL thread */
                        if (trd_priv_params[trd_num].thread_id == tid)
                        {
                            int retval = 0;
                            #define ECMD_DEL_TCS    2
                            extern sgx_enclave_id_t dynamo_eid;
                            ret = ecall_clear_tcs_dumb(dynamo_eid, &retval, ECMD_DEL_TCS);
                            if (SGX_SUCCESS != ret)
                            {
                                printf("SGX_ERROR, ret = %ld\n", ret);
                                assert(false);
                            }

                            memset(&trd_priv_params[trd_num], 0, sizeof(sgx_thread_priv_params));
                            g_trd_num_helper++;

                            break;
                        }
                        trd_num++;
                    }
                    pthread_mutex_unlock(&lock_m);
                }
            }
        }
    case SYS_close:
    case SYS_alarm:
    case SYS_fdatasync:
    case SYS_fsync:
    case SYS_umask:
    case SYS_dup:
    case SYS_getpgid:
    case SYS_epoll_create:
    case SYS_getsid:
    case SYS_setgid:
    case SYS_epoll_create1:
    case SYS_fchdir:
    case SYS_setuid:
    case SYS_inotify_init1:
    case SYS_setfsgid:
    case SYS_setfsuid:
        ret = syscall(sysno, N1);
        b = true;
        break;
    }

    echo_fun_return(sysno, b, __FUNCTION__, ret);

    return ret;
}

long ocall_syscall_1_S(long sysno, char *S1)
{
    long ret;
    bool b = false;

    switch (sysno)
    {
    case SYS_chdir:
    case SYS_chroot:
    case SYS_acct:
    case SYS_rmdir:
    case SYS_unlink:
    case SYS_shmdt:
    case SYS_swapoff:
    case SYS_mq_unlink:
        ret = syscall(sysno, S1);
        b = true;
        break;
    }

    echo_fun_return(sysno, b, __FUNCTION__, ret);

    return ret;
}

long ocall_syscall_1_Ti(long sysno, void *T1, int l1)
{
    long ret;
    bool b = false;

    switch (sysno)
    {
    case SYS_uname:
    case SYS_set_thread_area:
    case SYS_rt_sigsuspend:
        ret = syscall(sysno, T1);
        b = true;
        break;
    }

    echo_fun_return(sysno, b, __FUNCTION__, ret);

    return ret;
}

long ocall_syscall_1_To(long sysno, void *T1, int l1)
{
    long ret;
    bool b = false;

    switch (sysno)
    {
    case SYS_uname:
    case SYS_get_thread_area:
    case SYS_time:
    case SYS_times:
    case SYS_sysinfo:
    case SYS_pipe:
        ret = syscall(sysno, T1);
        b = true;
        break;
    }

    echo_fun_return(sysno, b, __FUNCTION__, ret);

    return ret;
}

long ocall_syscall_1_Tio(long sysno, void *T1, int l1)
{
    long ret;
    bool b = false;

    switch (sysno)
    {
    case SYS_set_tid_address:
        ret = syscall(sysno, T1);
        b = true;
        break;
    }

    echo_fun_return(sysno, b, __FUNCTION__, ret);

    return ret;
}

/*-----------------------------------syscalls with 2 parameters--------------------------*/
long ocall_syscall_2_NN(long sysno, long N1, long N2)
{
    long ret = 0;
    bool b = false;

    switch (sysno)
    {
    case SYS_munmap:
    case SYS_ftruncate:
    case SYS_arch_prctl:
    case SYS_dup2:
    case SYS_shutdown:
    case SYS_kill:
    case SYS_listen:
    case SYS_setpgid:
    case SYS_getpriority:
    case SYS_eventfd2:
    case SYS_flock:
    case SYS_fchmod:
    case SYS_setreuid:
    case SYS_setregid:
    case SYS_timerfd_create:
        ret = syscall(sysno, N1, N2);
        b = true;
        break;
    }

    echo_fun_return(sysno, b, __FUNCTION__, ret);

    return ret;
}

long ocall_syscall_2_NTi(long sysno, long N1, void *T2, int l2)
{
    long ret = 0;
    bool b = false;

    switch(sysno)
    {
        case SYS_setrlimit:
        case SYS_setgroups:
            ret = syscall(sysno, N1, T2);
            b = true;
            break;
    }

    echo_fun_return(sysno, b, __FUNCTION__, ret);

    return ret;
}

long ocall_syscall_2_NTo(long sysno, long N1, void *T2, int l2)
{
    long ret = 0;
    bool b = false;

    switch (sysno)
    {
    case SYS_getrlimit:
    case SYS_fstat:
    case SYS_fstatfs:
    case SYS_getitimer:
    case SYS_getrusage:
    case SYS_clock_gettime:
    case SYS_getgroups:
    case SYS_clock_getres:
        ret = syscall(sysno, N1, T2);
        b = true;
        break;
    }

    echo_fun_return(sysno, b, __FUNCTION__, ret);

    return ret;
}

long ocall_syscall_2_PiN(long sysno, void *P1, long N2, int dumb)
{
    long ret = 0;
    bool b = false;

    switch (sysno)
    {
    case SYS_mlock:
        ret = syscall(sysno, P1, N2);
        b = true;
        break;
    }

    echo_fun_return(sysno, b, __FUNCTION__, ret);

    return ret;
}

long ocall_syscall_2_PoN(long sysno, void *P1, long N2)
{
    long ret = 0;
    bool b = false;

    switch (sysno)
    {
    case SYS_getcwd:
        ret = syscall(sysno, P1, N2);
        b = true;
        break;
    }

    echo_fun_return(sysno, b, __FUNCTION__, ret);

    return ret;
}

long ocall_syscall_2_PioN(long sysno, void *P1, long N2)
{
    long ret = 0;
    bool b = false;

    switch (sysno)
    {
    case SYS_set_robust_list:
        ret = syscall(sysno, P1, N2);
        b = true;
        break;
    }

    echo_fun_return(sysno, b, __FUNCTION__, ret);

    return ret;
}

long ocall_syscall_2_SN(long sysno, const char *S1, long N2)
{
    long ret = 0;
    bool b = false;

    switch (sysno)
    {
    case SYS_mkdir:
    case SYS_access:
    case SYS_creat:
    case SYS_chmod:
    case SYS_umount2:
        ret = syscall(sysno, S1, N2);
        b = true;
        break;
    }

    echo_fun_return(sysno, b, __FUNCTION__, ret);

    return ret;
}

long ocall_syscall_2_STi(long sysno, const char *S1, void *T2, int l2)
{
    long ret = 0;
    bool b = false;

    switch (sysno)
    {
    case SYS_utime:
        ret = syscall(sysno, S1, T2);
        b = true;
        break;
    }

    echo_fun_return(sysno, b, __FUNCTION__, ret);

    return ret;
}

long ocall_syscall_2_STio(long sysno, const char *S1, void *T2, int l2)
{
    long ret = 0;
    bool b = false;

    switch (sysno)
    {
    case SYS_statfs:
        ret = syscall(sysno, S1, T2);
        b = true;
        break;
    }

    echo_fun_return(sysno, b, __FUNCTION__, ret);

    return ret;
}

long ocall_syscall_2_STo(long sysno, const char *S1, void *T2, int l2)
{
    long ret = 0;
    bool b = false;

    switch (sysno)
    {
    case SYS_stat:
    case SYS_lstat:
        ret = syscall(sysno, S1, T2);
        b = true;
        break;
    }

    echo_fun_return(sysno, b, __FUNCTION__, ret);

    return ret;
}

long ocall_syscall_2_TiN(long sysno, void *T1, int l1, long N2)
{
    long ret = 0;
    bool b = false;

    if (sysno == SYS_open)
    {
        ret = syscall(sysno, T1, N2);
        b = true;
    }

    echo_fun_return(sysno, b, __FUNCTION__, ret);

    return ret;
}

long ocall_syscall_2_ToN(long sysno, void *T1, int l1, long N2)
{
    long ret = 0;
    bool b = false;

    switch (sysno)
    {
    case SYS_gettimeofday:
    case SYS_pipe2:
        ret = syscall(sysno, T1, N2);
        b = true;
        break;
    }

    echo_fun_return(sysno, b, __FUNCTION__, ret);

    return ret;
}

long ocall_syscall_2_TiTo(long sysno, void *T1, void *T2, int l)
{
    long ret = 0;
    bool b = false;

    if (sysno == SYS_nanosleep) {
        ret = syscall(sysno, T1, T2);
        b = true;
    }

    echo_fun_return(sysno, b, __FUNCTION__, ret);

    return ret;
}

long ocall_syscall_2_SS(long sysno, const char *S1, const char *S2)
{
    long ret = 0;
    bool b = false;

    switch (sysno)
    {
    case SYS_link:
    case SYS_rename:
    case SYS_symlink:
        ret = syscall(sysno, S1, S2);
        b = true;
        break;
    }

    echo_fun_return(sysno, b, __FUNCTION__, ret);

    return ret;
}

/*-----------------------------------syscalls with 3 parameters--------------------------*/
long ocall_syscall_3_NNN(long sysno, long N1, long N2, long N3)
{
    long ret = 0;
    bool b = false;

    switch (sysno)
    {
    case SYS_tgkill:
    case SYS_lseek:
    case SYS_socket:
    case SYS_mprotect:
    case SYS_fcntl:
    case SYS_setpriority:
    case SYS_fchown:
    case SYS_setresuid:
    case SYS_setresgid:
    case SYS_ioperm:
    case SYS_dup3:
        ret = syscall(sysno, N1, N2, N3);
        b = true;
        break;
    }

    echo_fun_return(sysno, b, __FUNCTION__, ret);

    return ret;
}

long ocall_syscall_3_NNPi(long sysno, long N1, long N2, void *P3, int l3)
{
    long ret = 0;
    bool b = false;

    switch (sysno)
    {
    case SYS_ioctl:
        ret = syscall(sysno, N1, N2, P3);
        b = true;
        break;
    }

    echo_fun_return(sysno, b, __FUNCTION__, ret);

    return ret;
}

long ocall_syscall_3_NNPo(long sysno, long N1, long N2, void *P3, int l3)
{
    long ret = 0;
    bool b = false;

    switch (sysno)
    {
    case SYS_ioctl:

    /* FIX-ME: The addr argument N1 must be a multiple of the system page size. */
    case SYS_mincore:
        ret = syscall(sysno, N1, N2, P3);
        b = true;
        break;
    }

    echo_fun_return(sysno, b, __FUNCTION__, ret);

    return ret;
}

long ocall_syscall_3_NNPio(long sysno, long N1, long N2, void *P3, int l3)
{
    long ret = 0;
    bool b = false;

    switch (sysno)
    {
    case SYS_ioctl:
        ret = syscall(sysno, N1, N2, P3);
        b = true;
        break;
    }

    echo_fun_return(sysno, b, __FUNCTION__, ret);

    return ret;
}

long ocall_syscall_3_NNTi(long sysno, long N1, long N2, void *T3, int l3)
{
    long ret = 0;
    bool b = false;

    switch (sysno)
    {
    case SYS_fcntl:
        ret = syscall(sysno, N1, N2, T3);
        b = true;
        break;
    }

    echo_fun_return(sysno, b, __FUNCTION__, ret);

    return ret;
}

long ocall_syscall_3_NNTo(long sysno, long N1, long N2, void *T3, int l3)
{
    long ret = 0;
    bool b = false;

    switch (sysno)
    {
    case SYS_fcntl:
    case SYS_sched_getaffinity:
        ret = syscall(sysno, N1, N2, T3);
        b = true;
        break;
    }

    echo_fun_return(sysno, b, __FUNCTION__, ret);

    return ret;
}

long ocall_syscall_3_NPiN(long sysno, long N1, void *P2, long N3)
{
    long ret = 0;
    bool b = false;

    switch (sysno)
    {
    case SYS_write:
        ret = syscall(sysno, N1, P2, N3);
        b = true;
        /*sync();*/
    }

    echo_fun_return(sysno, b, __FUNCTION__, ret);

    return ret;
}

long ocall_syscall_3_NPoN(long sysno, long N1, void *P2, long N3)
{
    long ret = 0;
    bool b = false;

    switch (sysno)
    {
    case SYS_read:
    case SYS_connect:
    case SYS_bind:
        ret = syscall(sysno, N1, P2, N3);
        b = true;
        break;
    }

    echo_fun_return(sysno, b, __FUNCTION__, ret);

    return ret;
}

long ocall_syscall_3_NPioN(long sysno, long N1, void *P2, long N3)
{
    long ret = 0;
    bool b = false;

    switch (sysno)
    {
    case SYS_getdents:
    case SYS_getdents64:
        ret = syscall(sysno, N1, P2, N3);
        b = true;
        break;
    }

    echo_fun_return(sysno, b, __FUNCTION__, ret);

    return ret;
}

long ocall_syscall_3_NSN(long sysno, long N1, const char *S2,  long N3)
{
    long ret = 0;
    bool b = false;

    switch (sysno)
    {
    case SYS_inotify_add_watch:
    case SYS_mkdirat:
    case SYS_unlinkat:
        ret = syscall(sysno, N1, S2, N3);
        b = true;
        break;
    }

    echo_fun_return(sysno, b, __FUNCTION__, ret);

    return ret;
}

long ocall_syscall_3_NTiNP(long sysno, long N1, void *T2, int bsize, long N3, void *iovb, int len)
{
    long ret = 0;
    bool b = false;

    switch (sysno) 
    {
    case SYS_writev:
        {   
            struct iovec *iov = (struct iovec*)T2;
            int s_iov = sizeof(struct iovec);
            int c_iov = N3;
            unsigned long iov_bank = (unsigned long)iov;

            unsigned long iov_shd_addr = shadowing_iovec(iov, (char *)iovb, c_iov);
            assert(0 != iov_shd_addr);

            T2 = (void *)iov_shd_addr;
            ret = syscall(sysno, N1, T2, N3);
            T2 = (void *)iov_bank;

            free((char*)iov_shd_addr);
            iov_shd_addr = 0;

            b = true;
            break;
        }
    }

    echo_fun_return(sysno, b, __FUNCTION__, ret);

    return ret;
}

long ocall_syscall_3_NTiNPP(long sysno, long N1, void *T2, int l2, long N3, void *P4, int l4, void *P5, int l5)
{
    long ret = 0;
    bool b = false;

    switch (sysno)
    {
    case SYS_recvmsg:
        {
            /* do bank for struct msghdr to retrieve later on */
            struct msghdr *msg = (struct msghdr*)T2;
            unsigned long msg_name_bank = (unsigned long)msg->msg_name;
            unsigned long msg_iov_bank = (unsigned long)msg->msg_iov;

            /* substitute buffer addresses from the inside with the outside one */
            msg->msg_name = P4;
            msg->msg_namelen = l4;

            /* shadowing a nested struct iovec */
            int s_iov = sizeof(struct iovec);
            int c_iov = msg->msg_iovlen;
            struct iovec *iov_shd = (struct iovec*)malloc(c_iov * s_iov);
            assert(MAP_FAILED != iov_shd && NULL != iov_shd);
            memset(iov_shd, 0, c_iov * s_iov);   //cdd --
            unsigned long iov_shd_addr = (unsigned long)iov_shd;

            /* use the outside addresses to receive msg */
            iov_shd->iov_base = P5;
            iov_shd->iov_len = l5;

            /* do a substitution using shadowing iovec */
            msg->msg_iov = (struct _iovec *)iov_shd_addr;

            /* do the syscall */
            ret = syscall(sysno, N1, T2, N3);

            /* retrieve the inside buffer */
            msg->msg_name = (void*)msg_name_bank;
            msg->msg_iov = (struct _iovec *)msg_iov_bank;

            free((char*)iov_shd);
            iov_shd = NULL;

            b = true;
            break;
        }
    }

    echo_fun_return(sysno, b, __FUNCTION__, ret);

    return ret;
}

long ocall_syscall_3_NTiNPTi(long sysno, long N1, void *T2, int bsize, long N3, void *iovb, int l1, void *iovec, int l2)
{
    long ret = 0;
    bool b = false;

    switch (sysno)
    {
    case SYS_sendmsg:
        {
            struct msghdr *msg = (struct msghdr*)T2;
            struct iovec *iov = (struct iovec*)iovec;
            int s_msg = sizeof(struct msghdr);
            int s_iov = sizeof(struct iovec);
            int c_iov = msg->msg_iovlen;

            unsigned long msg_iov_bank = (unsigned long)msg->msg_iov;

            unsigned long msg_iov_shd_addr = shadowing_iovec(iov, (char *)iovb, c_iov);
            assert(0 != msg_iov_shd_addr);

            msg->msg_iov = (struct _iovec *)msg_iov_shd_addr;
            ret = syscall(sysno, N1, T2, N3);
            msg->msg_iov = (struct _iovec *)msg_iov_bank;

            free((char*)msg_iov_shd_addr);
            msg_iov_shd_addr = 0;

            b = true;
            break;
        }
    }

    echo_fun_return(sysno, b, __FUNCTION__, ret);

    return ret;
}

long ocall_syscall_3_NTiN(long sysno, long N1, void *T2, int l2, long N3)
{
    long ret = 0;
    bool b = false;

    switch (sysno)
    {
    case SYS_signalfd4:
        ret = syscall(sysno, N1, T2, N3);
        b = true;
        break;
    }

    echo_fun_return(sysno, b, __FUNCTION__, ret);

    return ret;
}

long ocall_syscall_3_NToN(long sysno, long N1, void *T2, int l2, long N3)
{
    long ret = 0;
    bool b = false;

    switch (sysno)
    {
    case SYS_readv:
        ret = syscall(sysno, N1, T2, N3);
        b = true;
        break;
    }

    echo_fun_return(sysno, b, __FUNCTION__, ret);

    return ret;
}

long ocall_syscall_3_NTiTo(long sysno, long N1, void *T2, int l2, void *T3, int l3)
{
    long ret = 0;
    bool b = false;

    switch (sysno)
    {
    case SYS_setitimer:
    case SYS_accept:
        ret = syscall(sysno, N1, T2, T3);
        b = true;
    }

    echo_fun_return(sysno, b, __FUNCTION__, ret);

    return ret;
}

long ocall_syscall_3_NToTio(long sysno, long N1, void *T2, int l2, void *T3, int l3)
{
    long ret = 0;
    bool b = false;

    switch (sysno)
    {
    case SYS_getpeername:
    case SYS_getsockname:
        ret = syscall(sysno, N1, T2, T3);

        b = true;
        break;
    }

    echo_fun_return(sysno, b, __FUNCTION__, ret);

    return ret;
}

long ocall_syscall_3_PiNN(long sysno, void *P1, long N2, long N3)
{
    long ret = 0;
    bool b = false;

    switch (sysno) 
    {
    case SYS_madvise:
        ret = syscall(sysno, P1, N2, N3);
        b = true;
        break;
    }

    echo_fun_return(sysno, b, __FUNCTION__, ret);

    return ret;
}

long ocall_syscall_3_PoNN(long sysno, void *P1, long N2, long N3)
{
    long ret = 0;
    bool b = false;

    switch (sysno) 
    {
    case SYS_getrandom:
        ret = syscall(sysno, P1, N2, N3);
        b = true;
        break;
    }

    echo_fun_return(sysno, b, __FUNCTION__, ret);

    return ret;
}

long ocall_syscall_3_PoPoPo(long sysno, void *P1, void *P2, void *P3, int l1)
{
    long ret = 0;
    bool b = false;

    switch (sysno)
    {
    case SYS_getresuid:
    case SYS_getresgid:
        ret = syscall(sysno, P1, P2, P3);
        b = true;
        break;
    }

    echo_fun_return(sysno, b, __FUNCTION__, ret);

    return ret;
}

long ocall_syscall_3_SNN(long sysno, const char *S1, long N2, long N3)
{
    long ret = 0;
    bool b = false;

    switch (sysno)
    {
    case SYS_open:
    case SYS_chown:
        ret = syscall(sysno, S1, N2, N3);
        b = true;
        break;
    }

    echo_fun_return(sysno, b, __FUNCTION__, ret);

    return ret;
}

long ocall_syscall_3_SNS(long sysno, const char *S1, long N2, const char *S3)
{
    long ret = 0;
    bool b = false;

    switch (sysno)
    {
    case SYS_symlinkat:
        ret = syscall(sysno, S1, N2, S3);
        b = true;
        break;
    }

    echo_fun_return(sysno, b, __FUNCTION__, ret);

    return ret;
}

long ocall_syscall_3_SPoN(long sysno, const char *S1, void *P2, long N3)
{
    long ret = 0;
    bool b = false;

    switch (sysno)
    {
    case SYS_readlink:
        ret = syscall(sysno, S1, P2, N3);
        b = true;
        break;
    }

    echo_fun_return(sysno, b, __FUNCTION__, ret);

    return ret;
}

long ocall_syscall_3_TioNN(long sysno, void *T1, int l1, long N2, long N3)
{
    long ret = 0;
    bool b = false;

    switch (sysno)
    {
    case SYS_poll:
        ret = syscall(sysno, T1, N2, N3);
        b = true;
        break;
    }

    echo_fun_return(sysno, b, __FUNCTION__, ret);

    return ret;
}

/*-----------------------------------syscalls with 4 parameters--------------------------*/
long ocall_syscall_4_NNNN(long sysno, long N1, long N2, long N3, long N4)
{
    long ret = 0;
    bool b = false;

    switch (sysno)
    {
    case SYS_mremap:
    case SYS_fadvise64:
        ret = syscall(sysno, N1, N2, N3, N4);
        b = true;
        break;
    }

    echo_fun_return(sysno, b, __FUNCTION__, ret);

    return ret;
}

long ocall_syscall_4_NNNTo(long sysno, long N1, long N2, long N3, void *T4, int l4)
{
    long ret = 0;
    bool b = false;

    if (sysno == SYS_socketpair)
    {
        ret = syscall(sysno, N1, N2, N3, T4);
        b = true;
    }

    echo_fun_return(sysno, b, __FUNCTION__, ret);

    return ret;
}

long ocall_syscall_4_NPiNN(long sysno, long N1, void *P2, int l2, long N3, long N4)
{
    long ret = 0;
    bool b = false;

    switch (sysno)
    {
    case SYS_pwrite64:
        ret = syscall(sysno, N1, P2, N3, N4);
        b = true;
        break;
    }

    echo_fun_return(sysno, b, __FUNCTION__, ret);

    return ret;
}

long ocall_syscall_4_NPoNN(long sysno, long N1, void *P2, int l2, long N3, long N4)
{
    long ret = 0;
    bool b = false;

    switch (sysno)
    {
    case SYS_pread64:
        ret = syscall(sysno, N1, P2, N3, N4);
        b = true;
        break;
    }

    echo_fun_return(sysno, b, __FUNCTION__, ret);

    return ret;
}

long ocall_syscall_4_NPiNTo(long sysno, long N1, void *P2, int l2, long N3, void* T4, long l4)
{
    long ret = 0;
    bool b = false;

    switch (sysno)
    {
    case SYS_wait4:
        ret = syscall(sysno, N1, P2, N3, T4);
        b = true;
        break;
    }

    echo_fun_return(sysno, b, __FUNCTION__, ret);

    return ret;
}

long ocall_syscall_4_NSNN(long sysno, long N1, const char *P2, long N3, long N4)
{
    long ret = 0;
    bool b = false;

    switch (sysno)
    {
    case SYS_faccessat:
    case SYS_openat:
        ret = syscall(sysno, N1, P2, N3, N4);
        b = true;
        break;
    }

    echo_fun_return(sysno, b, __FUNCTION__, ret);

    return ret;
}

long ocall_syscall_4_NToNN(long sysno, long N1, void *T2, int l2, long N3, long N4)
{
    long ret = 0;
    bool b = false;

    switch (sysno)
    {
    case SYS_sendmmsg:
        ret = syscall(sysno, N1, T2, N3, N4);
        b = true;
        break;
    }

    echo_fun_return(sysno, b, __FUNCTION__, ret);

    return ret;
}

long ocall_syscall_4_NTiToN(long sysno, long N1, void *T2, int l2, void *T3, int l3, long N4)
{
    long ret = 0;
    bool b = false;

    switch (sysno)
    {
    case SYS_rt_sigaction:
    case SYS_rt_sigprocmask:
    case SYS_accept4:
        ret = syscall(sysno, N1, T2, T3, N4);
        b = true;
        break;
    }

    echo_fun_return(sysno, b, __FUNCTION__, ret);

    return ret;
}

long ocall_syscall_4_NNNTio(long sysno, long N1, long N2, long N3, void* Tio, long l1)
{
    long ret = 0;
    bool b = false;

    switch (sysno) {
    case SYS_epoll_ctl:
        {
            ret = syscall(sysno, N1, N2, N3, Tio);
            b = true;
            break;
        }
    }

    echo_fun_return(sysno, b, __FUNCTION__, ret);

    return ret;
}

long ocall_syscall_4_NTioNN(long sysno, long N1, void* Tio, long N3, long N4, long l1)
{
    long ret = 0;
    bool b = false;

    switch (sysno) {
    case SYS_epoll_wait:
        {
            #define sizeof_epoll_event       12
            int size = N3 * sizeof_epoll_event;

            char *pevents = malloc(size);
            assert(MAP_FAILED != pevents && NULL != pevents);
            memset(pevents, 0, size);
            memcpy(pevents, Tio, size);

            ret = syscall(sysno, N1, (void*)pevents, N3, N4);

            if (ret > 0)
            {
                if (ret > 32)   assert(false && "too many events!!");
                memcpy(Tio, pevents, size);
            }

            free(pevents);
            pevents = NULL;
            
            b = true;
            break;
        }
    }

    echo_fun_return(sysno, b, __FUNCTION__, ret);

    return ret;
}

long ocall_syscall_4_NSPoN(long sysno, long N1, const char *S2, void *P3, long N4)
{
    long ret = 0;
    bool b = false;
    switch (sysno)
    {
    case SYS_readlinkat:
        ret = syscall(sysno, N1, S2, P3, N4);
        b = true;
        break;
    }

    echo_fun_return(sysno, b, __FUNCTION__, ret);

    return ret;
}

long ocall_syscall_4_NSToN(long sysno, long N1, const char *S2, void* T3, int l3, long N4)
{
    long ret = 0;
    bool b = false;
    switch (sysno)
    {
    case SYS_newfstatat:
        ret = syscall(sysno, N1, S2, T3, N4);
        b = true;
        break;
    }

    echo_fun_return(sysno, b, __FUNCTION__, ret);

    return ret;
}

long ocall_syscall_4_NSPioN(long sysno, long N1, const char *S2, void *P3, long N4)
{
    long ret = 0;
    bool b = false;
    switch (sysno)
    {
    case SYS_fgetxattr:
        ret = syscall(sysno, N1, S2, P3, N4);
        b = true;
        break;
    }

    echo_fun_return(sysno, b, __FUNCTION__, ret);

    return ret;
}

long ocall_syscall_4_SSPioN(long sysno, const char *S1, const char *S2, void* P3, long N4)
{
    long ret = 0;
    bool b = false;
    switch (sysno)
    {
    case SYS_getxattr:
    case SYS_lgetxattr:
        ret = syscall(sysno, S1, S2, P3, N4);
        b = true;
        break;
    }

    echo_fun_return(sysno, b, __FUNCTION__, ret);

    return ret;
}

long ocall_syscall_4_TioNTiTi(long sysno, void* T1, int l1, long N2, void* T3, int l3, void* T4, long l4)
{
    long ret = 0;
    bool b = false;
    switch (sysno)
    {
    case SYS_ppoll:
        ret = syscall(sysno, T1, N2, T3, T4);
        b = true;
        break;
    }

    echo_fun_return(sysno, b, __FUNCTION__, ret);

    return ret;
}

/*-----------------------------------syscalls with 5 parameters--------------------------*/
long ocall_syscall_5_NNNNN(long sysno, long N1, long N2, long N3, long N4, long N5)
{
    long ret = 0;
    bool b = false;

    switch (sysno)
    {
    case SYS_prctl:
        ret = syscall(sysno, N1, N2, N3, N4, N5);
        b = true;
        break;
    }

    echo_fun_return(sysno, b, __FUNCTION__, ret);

    return ret;
}

long ocall_syscall_5_NNNPiN(long sysno, long N1, long N2, long N3, void *P4, long N5)
{
    long ret = 0;
    bool b = false;
    switch (sysno)
    {
    case SYS_setsockopt:
        ret = syscall(sysno, N1, N2, N3, P4, N5);
        b = true;
        break;
    }

    echo_fun_return(sysno, b, __FUNCTION__, ret);

    return ret;
}

long ocall_syscall_5_NNNToTio(long sysno, long N1, long N2, long N3, void *T4, int l4, void *T5, int l5)
{
    long ret = 0;
    bool b = false;
    switch (sysno)
    {
    case SYS_getsockopt:
        ret = syscall(sysno, N1, N2, N3, T4, T5);
        b = true;
        break;
    }

    echo_fun_return(sysno, b, __FUNCTION__, ret);

    return ret;
}

#include "thread_helper.h"
long ocall_syscall_5_NPiNPoPoN(long sysno, unsigned long N1, void *Pi, unsigned long N2, size_t child_stack, int *Po1, int *Po2, unsigned long po12, unsigned long N3)
{
    long ret = 0;
    bool b = false;

    sgx_thread_priv_params trd_params;

    if (sysno == SYS_clone) {

        trd_params.flags = N1;
        trd_params.child_stack = N2;
        trd_params.ptid = (unsigned long)Po1;
        trd_params.ctid = (unsigned long)Po2;
        trd_params.pcid_inaddr = po12;
        trd_params.newtls = N3;

        extern sgx_enclave_id_t dynamo_eid;
        trd_params.current_eid = dynamo_eid;

        ret = thread_setup_agent(&trd_params);
        assert((ret > 0) && "thread setup failed!");
        
        b = true;
    }

    echo_fun_return(sysno, b, __FUNCTION__, ret);

    return ret;
}

long ocall_syscall_5_NSTiPiN(long sysno, long N1, const char *S2, void *T3, int l3, void *P4, int l4, long N5)
{
    long ret = 0;
    bool b = false;
    switch (sysno)
    {
    case SYS_name_to_handle_at:
        ret = syscall(sysno, N1, S2, T3, P4, N5);
        b = true;
        break;
    }

    echo_fun_return(sysno, b, __FUNCTION__, ret);

    return ret;
}

long ocall_syscall_5_NTioTioTioTi(long sysno, long N1, void *T2, void *T3, void *T4, int l1, void *T5, int l2)
{
    long ret = 0;
    bool b = false;

    if (sysno == SYS_select)
    {
        ret = syscall(sysno, N1, T2, T3, T4, T5);
        b = true;
    }

    echo_fun_return(sysno, b, __FUNCTION__, ret);

    return ret;
}

long ocall_syscall_5_NSPioNN(long sysno, long N1, const char *S2, void *P3, long N4, long N5)
{
    long ret = 0;
    bool b = false;
    switch (sysno)
    {
    case SYS_fsetxattr:
        ret = syscall(sysno, N1, S2, P3, N4, N5);
        b = true;
        break;
    }

    echo_fun_return(sysno, b, __FUNCTION__, ret);

    return ret;
}

long ocall_syscall_5_SSPioNN(long sysno, const char *S1, const char *S2, void *P3, long N4, long N5)
{
    long ret = 0;
    bool b = false;
    switch (sysno)
    {
    case SYS_setxattr:
    case SYS_lsetxattr:
        ret = syscall(sysno, S1, S2, P3, N4, N5);
        b = true;
        break;
    }

    echo_fun_return(sysno, b, __FUNCTION__, ret);

    return ret;
}

/*-----------------------------------syscalls with 5 parameters--------------------------*/
long ocall_syscall_6_NNNNNN(long sysno, long N1, long N2, long N3, long N4, long N5, long N6)
{
    long ret = 0;
    bool b = false;

    if (sysno == SYS_mmap)
    {
        ret = syscall(sysno, N1, N2, N3, N4, N5, N6);
        b = true;
    }

    echo_fun_return(sysno, b, __FUNCTION__, ret);

    return ret;
}

long ocall_syscall_6_NPiNNPiN(long sysno, long N1, void *P2, long N3, long N4, void *P5, long N6)
{
    long ret = 0;
    bool b = false;

    if (sysno == SYS_sendto)
    {
        ret = syscall(sysno, N1, P2, N3, N4, P5, N6);
        b = true;
    }

    echo_fun_return(sysno, b, __FUNCTION__, ret);

    return ret;
}

long ocall_syscall_6_NPoNNToTo(long sysno, long N1, void *P2, long N3, long N4, void *T5, int l5, void *T6, int l6)
{
    long ret = 0;
    bool b = false;

    if (sysno == SYS_recvfrom)
    {
        ret = syscall(sysno, N1, P2, N3, N4, T5, T6);
        b = true;
    }

    echo_fun_return(sysno, b, __FUNCTION__, ret);

    return ret;
}

long ocall_syscall_6_NPiPiPiTiTio(long sysno, long N1, void *P2, void *P3, void *P4, int l1, void *T5, int l5, void *T6, int l6)
{
    long ret = 0;
    bool b = false;
    switch (sysno)
    {
    case SYS_pselect6:
        ret = syscall(sysno, N1, P2, P3, P4, T5, T6);
        b = true;
        break;
    }

    echo_fun_return(sysno, b, __FUNCTION__, ret);

    return ret;
}

long ocall_syscall_6_TiNNTiNN(long sysno, long T1, int l1, long N2, long N3, void *T4, int l4, long N5, long N6)
{
    long ret = 0;
    bool b = false;

    if (sysno == SYS_mbind)
    {
        ret = syscall(sysno, T1, N2, N3, T4, N5, N6);
        b = true;
    }

    echo_fun_return(sysno, b, __FUNCTION__, ret);

    return ret;
}

long set_tid_ntrd(long sysno)
{
    long ret = 0;
    bool b = false;

    if (sysno == SYS_gettid) {
        ret = syscall(sysno);
        b = true;
    }

    echo_fun_return(sysno, b, __FUNCTION__, ret);

    return ret;
}