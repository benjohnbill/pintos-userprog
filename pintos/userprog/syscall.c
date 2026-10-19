#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/mmu.h"
#include "threads/thread.h"
#include "threads/loader.h"
#include "threads/vaddr.h"
#include "userprog/gdt.h"
#include "threads/flags.h"
#include "intrinsic.h"
#include "userprog/process.h"

void syscall_entry(void);
void syscall_handler(struct intr_frame *);

/* System call.
 *
 * Previously system call services was handled by the interrupt handler
 * (e.g. int 0x80 in linux). However, in x86-64, the manufacturer supplies
 * efficient path for requesting the system call, the `syscall` instruction.
 *
 * The syscall instruction works by reading the values from the the Model
 * Specific Register (MSR). For the details, see the manual. */

#define MSR_STAR 0xc0000081         /* Segment selector msr */
#define MSR_LSTAR 0xc0000082        /* Long mode SYSCALL target */
#define MSR_SYSCALL_MASK 0xc0000084 /* Mask for the eflags */

void syscall_init(void)
{
    write_msr(MSR_STAR,
              ((uint64_t)SEL_UCSEG - 0x10) << 48 | ((uint64_t)SEL_KCSEG) << 32);
    write_msr(MSR_LSTAR, (uint64_t)syscall_entry);

    /* The interrupt service rountine should not serve any interrupts
	 * until the syscall_entry swaps the userland stack to the kernel
	 * mode stack. Therefore, we masked the FLAG_FL. */
    write_msr(MSR_SYSCALL_MASK,
              FLAG_IF | FLAG_TF | FLAG_DF | FLAG_IOPL | FLAG_AC | FLAG_NT);
}

/* The main system call interface */
void syscall_handler(struct intr_frame *f)
{
    uint64_t syscall_num = f->R.rax;
    bool success = true;

    switch (syscall_num) {
    case SYS_WRITE: {
        int fd = (int)f->R.rdi;
        const void *buffer = (const void *)f->R.rsi;
        unsigned length = (unsigned)f->R.rdx;

        if (fd != STDOUT_FILENO) {
            f->R.rax = -1;
            break;
        }

        if (length == 0) {
            f->R.rax = 0;
            break;
        }

        if (buffer == NULL) {
            f->R.rax = -1;
            break;
        }

        /* 버퍼의 첫 바이트와 마지막 바이트가 모두 사용자 주소인지 본다. */
        if (!is_user_vaddr(buffer)) {
            f->R.rax = -1;
            break;
        }
        if (!is_user_vaddr((uintptr_t)buffer + length - 1)) {
            f->R.rax = -1;
            break;
        }

        /* 시작 페이지부터 마지막 페이지까지 각 페이지가 실제로 매핑되어 있는지 검사 */
        for (uintptr_t i = (uintptr_t)pg_round_down(buffer);
             i <= (uintptr_t)pg_round_down((uintptr_t)buffer + length - 1);
             i += PGSIZE) {
            if (pml4_get_page(thread_current()->pml4, (void *)i) == NULL) {
                f->R.rax = -1;
                success = false;
                break;
            }
        }

        /* 실제 기록한 바이트 수를 반환한다. */
        if (success) {
            putbuf((const char *)buffer, length);
            f->R.rax = length;
        }

        break;
    }

    /* TODO:
     1. process_wait((tid_t) f->R.rdi) 호출
     2. 반환된 자식의 status를 f->R.rax에 저장*/
    case SYS_WAIT: {
        tid_t child_tid = (tid_t)f->R.rdi;
        f->R.rax = process_wait(child_tid);
        break;
    }

    /* 종료 메시지는 process_exit()이 출력한다. 커널이 강제로 끝내는
     * 경로도 그곳을 지나기 때문이다. */
    case SYS_EXIT: {
        thread_current()->exit_status = (int)f->R.rdi;
        thread_exit();
    }

    /* 첫 번째 인자 레지스터에서 자식 이름을 받고
       현재 사용자 실행 문맥과 함꼐 process_fork()에 전달한다.
       반환된 자식 tid 또는 TID_ERROR를 반환 레지스터에 저장한다. */
    case SYS_FORK: {
        const char *thread_name = (const char *)f->R.rdi;
        f->R.rax = process_fork(thread_name, f);
        break;
    }
    }
}
