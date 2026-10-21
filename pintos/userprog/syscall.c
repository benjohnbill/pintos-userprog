#include "userprog/syscall.h"
#include <stdint.h>
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
#include "threads/palloc.h"
#include "userprog/process.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "devices/input.h"

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
        struct file *file = NULL;
        unsigned length = (unsigned)f->R.rdx;
        const void *buffer = (const void *)f->R.rsi;

        if (fd != STDOUT_FILENO) {
            file = process_find_file(fd);
            if (file == NULL) {
                f->R.rax = -1;
                break;
            }
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
        if (!is_user_vaddr(buffer) ||
            !is_user_vaddr((uintptr_t)buffer + length - 1)) {
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
            if (fd == STDOUT_FILENO) {
                putbuf((const char *)buffer, length);
                f->R.rax = length;
            } else {
                f->R.rax = file_write(file, buffer, length);
            }
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
    //! 정승영 코드 //
    case SYS_EXEC: {
        /* rdi는 현재 사용자 메모리에 있는 명령줄 주소다.
         * process_exec()가 기존 주소 공간을 없애므로 커널 페이지에 복사한다. */
        const char *user_cmd = (const char *)f->R.rdi;
        char *cmd_copy = palloc_get_page(0);
        int i = 0;

        /* 할당에 실패했다면 해제할 페이지 없이 비정상 종료한다. */
        if (cmd_copy == NULL) {
            thread_current()->exit_status = -1;
            thread_exit();
        }

        /* 널 종료 문자까지 복사하되 커널 페이지 한 장을 넘지 않는다.
         * 사용자 메모리는 읽기 전에 주소 범위와 실제 매핑을 확인한다. */
        for (; i < PGSIZE; i++) {
            if (!is_user_vaddr(user_cmd + i)) {
                palloc_free_page(cmd_copy);
                thread_current()->exit_status = -1;
                thread_exit();
            }
            /* 사용자 주소 범위 안이어도 매핑되지 않았을 수 있다. */
            if (!pml4_get_page(thread_current()->pml4, user_cmd + i)) {
                palloc_free_page(cmd_copy);
                thread_current()->exit_status = -1;
                thread_exit();
            }
            cmd_copy[i] = user_cmd[i];
            /* 끝 문자까지 복사해야 새 명령줄이 C 문자열이 된다. */
            if (user_cmd[i] == '\0') {
                break;
            }
        }
        /* 한 페이지 안에서 끝 문자를 못 찾았다면 적재할 수 없다. */
        if (i == PGSIZE) {
            palloc_free_page(cmd_copy);
            thread_current()->exit_status = -1;
            thread_exit();
        }
        /* process_exec()이 복사 페이지를 해제한다. 성공하면 돌아오지 않고,
         * 적재 실패로 돌아오면 기존 주소 공간이 없으므로 종료한다. */
        if (process_exec(cmd_copy) == -1) {
            thread_current()->exit_status = -1;
            thread_exit();
        }
    }
        //! 끝 //

    /* 로직
       1. f->R.rdi에서 사용자 파일명 포인터를 받는다.
       2. 파일명이 NULL인지, 사용자 주소인지, NULL 문자까지 매핑되었는지 검사한다.
       3. filesys_open()으로 struct *file을 얻는다.
       4. 실패하면 -1 반환
       5. 성공하면 process_add_file()로 fd를 발급한다.
       6. fd 발급이 실패했다면 방금 연 파일을 닫는다.
       7. 성공한 fd를 f->R.rax에 저장 */
    case SYS_OPEN: {
        const char *file_name = (const char *)f->R.rdi;
        size_t file_idx;
        struct file *file;
        int fd;

        if (file_name == NULL) {
            thread_current()->exit_status = -1;
            thread_exit();
        }

        for (file_idx = 0; file_idx < PGSIZE; file_idx++) {
            if (!is_user_vaddr(file_name + file_idx)) {
                thread_current()->exit_status = -1;
                thread_exit();
            }

            if (!pml4_get_page(thread_current()->pml4, file_name + file_idx)) {
                thread_current()->exit_status = -1;
                thread_exit();
            }

            if (file_name[file_idx] == '\0') {
                break;
            }
        }

        if (file_idx == PGSIZE) {
            thread_current()->exit_status = -1;
            thread_exit();
        }

        file = filesys_open(file_name);
        if (file == NULL) {
            f->R.rax = -1;
            break;
        }
        fd = process_add_file(file);
        if (fd == -1) {
            file_close(file);
        }

        f->R.rax = fd;
        break;
    }

    /* 로직
       1. f->R.rdi에서 int fd를 꺼낸다
       2. process_close_file(fd)를 호출한다.
       3. 반환값은 사용자에게 돌려줄 필요가 없다.
       4. 존재하지 않는 fd라면 process_close_file()이 false를 반환하지만 무시 가능. */
    case SYS_CLOSE: {
        int fd = (int)f->R.rdi;

        process_close_file(fd);
        break;
    }

    /* 로직
       1. 시작 주소가 사용자 주소인지 확인
       2. 마지막 주소 buffer + length -1을 계산
       3. 주소 계산에서 정수 오버플로가 발생하지 않았는지 확인
       4. 마지막 주소도 사용자 주소인지 확인
       5. 시작 페이지부터 마지막 페이지까지 매핑 여부 확인*/
    case SYS_READ: {
        int fd = (int)f->R.rdi;
        void *buffer = (void *)f->R.rsi;
        unsigned length = (unsigned)f->R.rdx;
        struct file *file;
        uint64_t *vaddr;

        if (length == 0) {
            f->R.rax = 0;
            break;
        }
        if (buffer == NULL) {
            thread_current()->exit_status = -1;
            thread_exit();
        }

        if (!is_user_vaddr(buffer)) {
            thread_current()->exit_status = -1;
            thread_exit();
        }
        if ((uintptr_t)buffer + length - 1 < (uintptr_t)buffer) {
            thread_current()->exit_status = -1;
            thread_exit();
        }

        if (!is_user_vaddr((void *)((uintptr_t)buffer + length - 1))) {
            thread_current()->exit_status = -1;
            thread_exit();
        }

        for (uintptr_t page_addr = (uintptr_t)pg_round_down(buffer);
             page_addr <=
             (uintptr_t)pg_round_down((uintptr_t)buffer + length - 1);
             page_addr += PGSIZE) {
            if (pml4_get_page(thread_current()->pml4, (void *)page_addr) ==
                NULL) {
                thread_current()->exit_status = -1;
                thread_exit();
            }

            vaddr = pml4e_walk(thread_current()->pml4, page_addr, 0);
            if (vaddr) {
                if (!is_writable(vaddr)) {
                    thread_current()->exit_status = -1;
                    thread_exit();
                }
            }
        }

        if (fd == STDOUT_FILENO) {
            f->R.rax = -1;
            break;
        }

        if (fd == STDIN_FILENO) {
            for (unsigned i = 0; i < length; i++) {
                ((uint8_t *)buffer)[i] = input_getc();
            }
            f->R.rax = length;
            break;
        }

        file = process_find_file(fd);
        if (file == NULL) {
            f->R.rax = -1;
            break;
        }

        f->R.rax = file_read(file, buffer, length);
        break;

    case SYS_FILESIZE: {
        int fd = (int)f->R.rdi;
        struct file *file = process_find_file(fd);

        if (file == NULL) {
            f->R.rax = -1;
            break;
        }

        f->R.rax = file_length(file);
        break;
    }

    case SYS_TELL: {
        int fd = (int)f->R.rdi;
        struct file *file = process_find_file(fd);

        if (file == NULL) {
            f->R.rax = -1;
            break;
        }

        f->R.rax = file_tell(file);
        break;
    }

    /* 열린 파일에서 다음에 읽거나 쓸 위치를 사용자가 원하는 곳으로 옮기는 시스템 콜 */
    case SYS_SEEK: {
        int fd;
        unsigned
            position; // 다음 read() 또는 write()가 시작 될 바이트 위치. 파일 오프셋
        struct file *file;

        fd = (int)f->R.rdi;
        position = (unsigned)f->R.rsi;

        file = process_find_file(fd);

        if (file != NULL) {
            file_seek(file, position);
        }
        break;
    }
    }
    }
}
