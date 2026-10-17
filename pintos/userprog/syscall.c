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

/* TODO:
 * f->R.rax에서 시스템 콜 번호를 읽는지 확인
 * f->R.rdi, f->R.rsi, f->R.rdx에서 write() 인자를 읽을 수 있는지 확인
 * 현재 기본 코드가 시스템 콜을 어떻게  처리하는지 확인 */

/* 핸들러의 로직
 1. f->R.rax에서 시스템 콜 번호 읽기
 2. switch문으로 번호 분기
 3. SYS_WRITE인 경우 fd, buffer, size 읽기 (각 시스템 콜 번호 마다 다른 것을 읽겠지?)
 4. 처리 결과를 f->R.rax에 기록
 5. syscall_handler() 반환 */
/* The main system call interface */
void syscall_handler(struct intr_frame *f)
{
    // TODO: Your implementation goes here.
    uint64_t syscall_num;
    syscall_num = f->R.rax;
    bool success = true;

    switch (syscall_num) {
    case SYS_WRITE: {
        /* SYS_WRITE의 흐름
     1. f->R.rdi에서 fd 추출
     2. f->R.rsi에서 buffer 추출
     3. f->R.rdx에서 size 추출
     4. fd == 1인지 확인
     5. size == 0 처리
     6. buffer 전체 범위 검증
     7. 유효한 putbuf(buffer, size)
     8. 실제 기록한 바이트 수를 f->R.rax에 저장
     9. break */
        int fd = (int)f->R.rdi;
        const void *buffer = (const void *)f->R.rsi;
        unsigned size = (unsigned)f->R.rdx;

        if (fd != 1) {
            f->R.rax = -1;
            break;
        }

        if (size == 0) {
            f->R.rax = 0;
            break;
        }

        /* NULL이 아닌지 확인 */
        if (buffer == NULL) {
            f->R.rax = -1;
            break;
        }

        /* 시작 주소가 사용자 주소인지 is_user_vaddr()로 확인 */
        if (!is_user_vaddr(buffer)) {
            f->R.rax = -1;
            break;
        }

        /* 마지막 주소 buffer + size -1도 사용자 주소인지 확인(buffer는 보이드 포인터 타입이라 형변환 필요) */
        if (!is_user_vaddr((uintptr_t)buffer + size - 1)) {
            f->R.rax = -1;
            break;
        }

        /* 시작 페이지부터 마지막 페이지까지 각 페이지가 실제로 매핑되어 있는지 검사 */
        for (uintptr_t i = (uintptr_t)pg_round_down(buffer);
             i <= (uintptr_t)pg_round_down((uintptr_t)buffer + size - 1);
             i += PGSIZE) {
            if (pml4_get_page(thread_current()->pml4, (void *)i) == NULL) {
                f->R.rax = -1;
                success = false;
                break;
            }
        }

        /* 실제 기록한 바이트를 f->R.rax에 저장하고 반환 */
        if (success) {
            putbuf((const char *)buffer, size);
            f->R.rax = size;
        }

        break;
    }

    /* SYS_EXIT의 흐름
     1. f->R.rdi에서 status를 꺼낸다.
     2. 현재 스레드의 이름을 가져온다.
     3. Pintos가 요구하는 종료 메시지를 출력한다.
     4. 현재 사용자 프로세스를 종료한다.
     5. 핸들러에서 thread_exit()도 처리한다. */
    case SYS_EXIT: {
        int status = (int)f->R.rdi;
        printf("%s: exit(%d)\n", thread_current()->name, status);
        thread_exit();
    }
    }

    // printf("system call!\n");
    // thread_exit();
}
