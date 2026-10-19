#include "userprog/process.h"
#include <debug.h>
#include <inttypes.h>
#include <round.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "userprog/gdt.h"
#include "userprog/tss.h"
#include "filesys/directory.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "threads/flags.h"
#include "threads/init.h"
#include "threads/interrupt.h"
#include "threads/malloc.h"
#include "threads/palloc.h"
#include "threads/thread.h"
#include "threads/mmu.h"
#include "threads/vaddr.h"
#include "intrinsic.h"
//! 정승영 코드 //
/* 자식 종료를 기다리고 알릴 때 사용할 세마포어 함수의 선언. */
#include "threads/synch.h"
//! 끝 //
#ifdef VM
#include "vm/vm.h"
#endif

//! 정승영 코드 //
/*
 * 자식 한 명의 종료 기록. 부모의 children 목록에 들어가고,
 * 자식의 thread.child_info도 같은 기록을 가리킨다.
 * 자식의 thread 구조체가 사라져도 부모가 종료 상태를 읽을 수 있도록
 * 별도로 할당한다. 현재는 부모가 wait()에서 읽은 뒤 해제한다. */
struct child_info {
    tid_t tid;                  /* 부모가 기다릴 자식을 찾는 식별자. */
    int status;                 /* 자식의 exit_status를 부모에게 전할 값. */
    struct semaphore exit_sema; /* 자식의 종료 통지를 기다리는 세마포어. */
    struct list_elem elem;      /* 부모의 children 목록에 넣는 연결 고리. */

};
/*
 * thread_create()의 마지막 인자는 포인터 하나뿐이다.
 * 따라서 자식 initd()에 필요한 두 주소를 하나로 묶어 전달한다. */
struct initd_args {
    char *fn_copy;                  /* 복사해 둔 명령줄 페이지의 주소. */
    struct child_info *child_info;  /* 부모와 자식이 공유할 종료 기록. */
};
//! 끝 //

static void process_cleanup(void);
/* const char *file_name -> char *file_name 로 변경 */
static bool load(char *file_name, struct intr_frame *if_);
static void initd(void *f_name);
static void __do_fork(void *);
/* General process initializer for initd and other process. */
static void process_init(void)
{
    struct thread *current = thread_current();
}

/* Starts the first userland program, called "initd", loaded from FILE_NAME.
 * The new thread may be scheduled (and may even exit)
 * before process_create_initd() returns. Returns the initd's
 * thread id, or TID_ERROR if the thread cannot be created.
 * Notice that THIS SHOULD BE CALLED ONCE. */
//! 정승영 코드 //
tid_t process_create_initd(const char *file_name)
{
    tid_t tid;
    char *fn_copy;
    struct child_info *child_info;
    struct initd_args *args;
    char thread_name[16];

    /* 자식이 실행되기 전에 부모의 원본 명령줄이 바뀔 수 있으므로 복사한다.
     * palloc_get_page()로 얻은 이 페이지는 나중에 process_exec()가 해제한다. */
    fn_copy = palloc_get_page(0);

    if (fn_copy == NULL) {
        return TID_ERROR;
    }

    /* 스레드 구조체와 별개로 종료 기록을 할당한다.
     * 자식 스레드가 먼저 사라져도 부모가 이 기록으로 결과를 읽는다. */
    child_info = malloc(sizeof(struct child_info));

    if (child_info == NULL) {
        /* 두 번째 할당에 실패했으므로 먼저 확보한 명령줄 페이지만 반납한다. */
        palloc_free_page(fn_copy);
        return TID_ERROR;
    }

    /* 초기값 0: 자식이 sema_up()하기 전까지 부모의 sema_down()은 대기한다.
     * 자식이 먼저 알렸다면 값이 남아 나중의 sema_down()이 바로 통과한다. */
    sema_init(&child_info->exit_sema, 0);
    strlcpy(fn_copy, file_name, PGSIZE);

    /* thread_create()에 넘길 두 주소를 담는 전달용 구조체를 만든다. */
    args = malloc(sizeof(struct initd_args));
    if (args == NULL) {
        /* 전달용 구조체를 만들지 못했으므로 앞서 할당한 자원도 반납한다. */
        palloc_free_page(fn_copy);
        free(child_info);
        return TID_ERROR;
    }
    args->fn_copy = fn_copy;
    args->child_info = child_info;

    /* 최초 실행 때만 명령줄 첫 단어를 스레드 이름으로 사용한다.
     * 인자 전달에 쓸 fn_copy는 수정하지 않는다. */
    const char *program_name = file_name;
    /* 첫 공백 전까지가 실행 파일 이름이다. 스레드 이름은 최대 15자다. */
    size_t name_len = strcspn(program_name, " ");
    if (name_len >= sizeof thread_name) {
        name_len = sizeof thread_name - 1;
    }
    /* memcpy()는 끝 문자를 복사하지 않으므로 직접 붙인다. */
    memcpy(thread_name, program_name, name_len);
    thread_name[name_len] = '\0';

    /* 새 스레드의 시작 함수 initd()에 전달용 구조체의 주소 하나를 넘긴다. */
    tid = thread_create(thread_name, PRI_DEFAULT, initd, args);
    if (tid == TID_ERROR) {
        /* 새 스레드가 없으므로 세 할당의 소유권은 여전히 이 함수에 있다. */
        palloc_free_page(fn_copy);
        free(child_info);
        free(args);
        return TID_ERROR;
    }
    /* thread_create()가 성공한 뒤에야 실제 자식 tid를 알 수 있다.
     * 자식이 이미 실행되거나 종료됐더라도 기록은 아직 부모가 보유한다. */
    child_info->tid = tid;

    /* 부모가 process_wait(tid)에서 이 자식의 종료 기록(child_info)을
     * 찾을 수 있도록 부모의 children 목록에 연결한다. */
    list_push_back(&thread_current()->children, &child_info->elem);
    return tid;
}
//! 끝 //

/* A thread function that launches first user process. */
static void initd(void *f_name)
{
#ifdef VM
    supplemental_page_table_init(&thread_current()->spt);
#endif
    //! 정승영 코드 //
    /* thread_create()의 마지막 인자로 받은 void *를 원래 타입으로 읽는다. */
    struct initd_args *args = f_name;

    /* 부모가 만든 종료 기록을 자식도 가리키게 한다.
     * 이후 process_exit()가 이 주소를 통해 상태를 기록하고 부모를 깨운다. */
    thread_current()->child_info = args->child_info;
    /* 전달용 구조체를 해제한 뒤에도 명령줄 주소를 사용할 수 있게 보관한다. */
    char *fn_copy = args->fn_copy;
    /* 전달용 구조체만 해제한다. fn_copy와 child_info는 별도로 할당됐다. */
    free(args);
    process_init();

    /* process_exec()가 명령줄 페이지를 사용한 뒤 해제한다.
     * 성공하면 유저 모드로 전환하므로 여기로 돌아오지 않는다. */
    if (process_exec(fn_copy) < 0) {
        PANIC("Fail to launch initd\n");
    }
    //! 끝 //
    NOT_REACHED();
}

/* Clones the current process as `name`. Returns the new process's thread id, or
 * TID_ERROR if the thread cannot be created. */
tid_t process_fork(const char *name, struct intr_frame *if_ UNUSED)
{
    /* Clone current thread to new thread.*/
    return thread_create(name, PRI_DEFAULT, __do_fork, thread_current());
}

#ifndef VM
/* Duplicate the parent's address space by passing this function to the
 * pml4_for_each. This is only for the project 2. */
static bool duplicate_pte(uint64_t *pte, void *va, void *aux)
{
    struct thread *current = thread_current();
    struct thread *parent = (struct thread *)aux;
    void *parent_page;
    void *newpage;
    bool writable;

    /* 1. TODO: If the parent_page is kernel page, then return immediately. */

    /* 2. Resolve VA from the parent's page map level 4. */
    parent_page = pml4_get_page(parent->pml4, va);

    /* 3. TODO: Allocate new PAL_USER page for the child and set result to
	 *    TODO: NEWPAGE. */

    /* 4. TODO: Duplicate parent's page to the new page and
	 *    TODO: check whether parent's page is writable or not (set WRITABLE
	 *    TODO: according to the result). */

    /* 5. Add new page to child's page table at address VA with WRITABLE
	 *    permission. */
    if (!pml4_set_page(current->pml4, va, newpage, writable)) {
        /* 6. TODO: if fail to insert page, do error handling. */
    }
    return true;
}
#endif

/* A thread function that copies parent's execution context.
 * Hint) parent->tf does not hold the userland context of the process.
 *       That is, you are required to pass second argument of process_fork to
 *       this function. */
static void __do_fork(void *aux)
{
    struct intr_frame if_;
    struct thread *parent = (struct thread *)aux;
    struct thread *current = thread_current();
    /* TODO: somehow pass the parent_if. (i.e. process_fork()'s if_) */
    struct intr_frame *parent_if;
    bool succ = true;

    /* 1. Read the cpu context to local stack. */
    memcpy(&if_, parent_if, sizeof(struct intr_frame));

    /* 2. Duplicate PT */
    current->pml4 = pml4_create();
    if (current->pml4 == NULL) {
        goto error;
    }

    process_activate(current);
#ifdef VM
    supplemental_page_table_init(&current->spt);
    if (!supplemental_page_table_copy(&current->spt, &parent->spt)) {
        goto error;
    }
#else
    if (!pml4_for_each(parent->pml4, duplicate_pte, parent)) {
        goto error;
    }
#endif

    /* TODO: Your code goes here.
	 * TODO: Hint) To duplicate the file object, use `file_duplicate`
	 * TODO:       in include/filesys/file.h. Note that parent should not return
	 * TODO:       from the fork() until this function successfully duplicates
	 * TODO:       the resources of parent.*/

    process_init();

    /* Finally, switch to the newly created process. */
    if (succ) {
        do_iret(&if_);
    }
error:
    thread_exit();
}

/* Switch the current execution context to the f_name.
 * Returns -1 on fail. */
int process_exec(void *f_name)
{
    char *file_name = f_name;
    bool success;

    /* We cannot use the intr_frame in the thread structure.
	 * This is because when current thread rescheduled,
	 * it stores the execution information to the member. */
    struct intr_frame _if;
    _if.ds = _if.es = _if.ss = SEL_UDSEG;
    _if.cs = SEL_UCSEG;
    _if.eflags = FLAG_IF | FLAG_MBS;

    /* We first kill the current context */
    process_cleanup();

    /* And then load the binary */
    success = load(file_name, &_if);

    /* If load failed, quit. */
    palloc_free_page(file_name);
    if (!success) {
        return -1;
    }

    /* Start switched process. */
    do_iret(&_if);
    NOT_REACHED();
}

/* Waits for thread TID to die and returns its exit status.  If
 * it was terminated by the kernel (i.e. killed due to an
 * exception), returns -1.  If TID is invalid or if it was not a
 * child of the calling process, or if process_wait() has already
 * been successfully called for the given TID, returns -1
 * immediately, without waiting.
 *
 * 현재 구현은 초기 프로세스의 종료 대기를 위한 기초 단계다.
 * fork로 만든 자식과 부모의 선종료 등은 이후에 확장해야 한다. */
//! 정승영 코드 //
int process_wait(tid_t child_tid)
{
    /* 이 함수를 호출한 부모 스레드의 자식 목록만 검사한다.
     * 따라서 다른 부모의 자식 tid로는 기다릴 수 없다. */
    struct thread *parent = thread_current();

    /* list에는 child_info 자체가 아니라 그 안의 elem이 들어 있다. */
    for (struct list_elem *e = list_begin(&parent->children);
        e != list_end(&parent->children); e = list_next(e)) {
            /* elem의 주소로 원래 child_info 구조체의 주소를 얻는다. */
            struct child_info *info = list_entry(e, struct child_info, elem);
            if (info->tid == child_tid) {
                /* 자식이 종료를 알릴 때까지 부모를 재운다.
                 * 이미 알렸다면 세마포어 값 덕분에 바로 진행한다. */
                sema_down(&info->exit_sema);
                /* 자식이 기록한 결과를 해제 전에 지역 변수로 복사한다. */
                int status = info->status;
                /* 한 번 기다린 자식의 기록을 목록에서 제거하고 해제한다.
                 * 같은 tid로 다시 wait하면 목록에서 찾지 못해 -1이 된다. */
                list_remove(&info->elem);
                free(info);
                return status;
            }
        }
    /* 해당 tid가 직접 만든 자식 목록에 없으면 기다리지 않는다. */
    return -1;
}
//! 끝 //

/* Exit the process. This function is called by thread_exit (). */
void process_exit(void)
{
    struct thread *curr = thread_current();

    /* Kernel threads have no page table; only user processes print. */
    if (curr->pml4 != NULL) {
        printf("%s: exit(%d)\n", curr->name, curr->exit_status);
    }
    /* 프로세스 자원을 정리한 다음 부모에게 종료를 알린다. */

    process_cleanup();
    //! 정승영 코드 //
    /* 부모와 공유하는 종료 기록이 있는 자식만 부모에게 통지한다.
     * 일반 커널 스레드는 child_info가 NULL이므로 이 분기를 건너뛴다. */
    if (curr->child_info != NULL) {
        /* 먼저 상태를 기록해야, 깨어난 부모가 올바른 값을 읽는다. */
        curr->child_info->status = curr->exit_status;
        /* process_wait()에서 잠든 부모를 깨우거나 통지를 남긴다. */
        sema_up(&curr->child_info->exit_sema);
    }
    //! 끝 //

}

/* Free the current process's resources. */
static void process_cleanup(void)
{
    struct thread *curr = thread_current();

#ifdef VM
    supplemental_page_table_kill(&curr->spt);
#endif

    uint64_t *pml4;
    /* Destroy the current process's page directory and switch back
	 * to the kernel-only page directory. */
    pml4 = curr->pml4;
    if (pml4 != NULL) {
        /* Correct ordering here is crucial.  We must set
		 * cur->pagedir to NULL before switching page directories,
		 * so that a timer interrupt can't switch back to the
		 * process page directory.  We must activate the base page
		 * directory before destroying the process's page
		 * directory, or our active page directory will be one
		 * that's been freed (and cleared). */
        curr->pml4 = NULL;
        pml4_activate(NULL);
        pml4_destroy(pml4);
    }
}

/* Sets up the CPU for running user code in the nest thread.
 * This function is called on every context switch. */
void process_activate(struct thread *next)
{
    /* Activate thread's page tables. */
    pml4_activate(next->pml4);

    /* Set thread's kernel stack for use in processing interrupts. */
    tss_update(next);
}

/* We load ELF binaries.  The following definitions are taken
 * from the ELF specification, [ELF1], more-or-less verbatim.  */

/* ELF types.  See [ELF1] 1-2. */
#define EI_NIDENT 16

#define PT_NULL 0           /* Ignore. */
#define PT_LOAD 1           /* Loadable segment. */
#define PT_DYNAMIC 2        /* Dynamic linking info. */
#define PT_INTERP 3         /* Name of dynamic loader. */
#define PT_NOTE 4           /* Auxiliary info. */
#define PT_SHLIB 5          /* Reserved. */
#define PT_PHDR 6           /* Program header table. */
#define PT_STACK 0x6474e551 /* Stack segment. */

#define PF_X 1 /* Executable. */
#define PF_W 2 /* Writable. */
#define PF_R 4 /* Readable. */

/* Executable header.  See [ELF1] 1-4 to 1-8.
 * This appears at the very beginning of an ELF binary. */
struct ELF64_hdr {
    unsigned char e_ident[EI_NIDENT];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint64_t e_entry;
    uint64_t e_phoff;
    uint64_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
};

struct ELF64_PHDR {
    uint32_t p_type;
    uint32_t p_flags;
    uint64_t p_offset;
    uint64_t p_vaddr;
    uint64_t p_paddr;
    uint64_t p_filesz;
    uint64_t p_memsz;
    uint64_t p_align;
};

/* Abbreviations */
#define ELF ELF64_hdr
#define Phdr ELF64_PHDR

static bool setup_stack(struct intr_frame *if_);
static bool validate_segment(const struct Phdr *, struct file *);
static bool load_segment(struct file *file, off_t ofs, uint8_t *upage,
                         uint32_t read_bytes, uint32_t zero_bytes,
                         bool writable);

/* 수정 가능한 FILE_NAME 복사본에서 ELF 실행 파일을 읽는다.
 * 시작 주소는 RIP, 초기 스택 주소는 RSP에 저장한다.
 * 성공하면 true, 실패하면 false를 반환한다. */
static bool load(char *file_name, struct intr_frame *if_)
{
    struct thread *t = thread_current();
    struct ELF ehdr;
    struct file *file = NULL;
    off_t file_ofs;
    bool success = false;

    /* 사용자 스택에 복사한 문자열의 주소를 임시로 보관한다. */
    char **arg_addrs = NULL;
    int argc = 0;
    int i;

    /* Allocate and activate page directory. */
    t->pml4 = pml4_create();
    if (t->pml4 == NULL) {
        goto done;
    }
    process_activate(thread_current());

    /* 명령줄에서 실행 파일 이름을 분리해 연다. */
    int arg_cnt = 0;

    /* strtok_r()가 원본을 바꾸기 전에 단어 수를 세어 임시 배열 크기를 정한다. */
    for (i = 0; file_name[i] != '\0'; i++) {
        if (file_name[i] != ' ' && (i == 0 || file_name[i - 1] == ' ')) {
            arg_cnt++;
        }
    }

    /* 복사된 명령줄을 나누고, 첫 토큰을 실행 파일 이름으로 사용한다. */
    char *save_ptr;
    char *token = strtok_r(file_name, " ", &save_ptr);

    if (token == NULL) {
        goto done;
    }

    /* 문자열이 아닌, 각 문자열의 사용자 스택 주소만 저장할 공간이다. */
    arg_addrs = malloc(arg_cnt * sizeof *arg_addrs);
    if (arg_addrs == NULL) {
        goto done;
    }
    file = filesys_open(token);

    if (file == NULL) {
        printf("load: %s: open failed\n", file_name);
        goto done;
    }

    /* Read and verify executable header. */
    if (file_read(file, &ehdr, sizeof ehdr) != sizeof ehdr ||
        memcmp(ehdr.e_ident, "\177ELF\2\1\1", 7) || ehdr.e_type != 2 ||
        ehdr.e_machine != 0x3E // amd64
        || ehdr.e_version != 1 || ehdr.e_phentsize != sizeof(struct Phdr) ||
        ehdr.e_phnum > 1024) {
        printf("load: %s: error loading executable\n", file_name);
        goto done;
    }

    /* Read program headers. */
    file_ofs = ehdr.e_phoff;
    for (i = 0; i < ehdr.e_phnum; i++) {
        struct Phdr phdr;

        if (file_ofs < 0 || file_ofs > file_length(file)) {
            goto done;
        }
        file_seek(file, file_ofs);

        if (file_read(file, &phdr, sizeof phdr) != sizeof phdr) {
            goto done;
        }
        file_ofs += sizeof phdr;
        switch (phdr.p_type) {
        case PT_NULL:
        case PT_NOTE:
        case PT_PHDR:
        case PT_STACK:
        default:
            /* Ignore this segment. */
            break;
        case PT_DYNAMIC:
        case PT_INTERP:
        case PT_SHLIB:
            goto done;
        case PT_LOAD:
            if (validate_segment(&phdr, file)) {
                bool writable = (phdr.p_flags & PF_W) != 0;
                uint64_t file_page = phdr.p_offset & ~PGMASK;
                uint64_t mem_page = phdr.p_vaddr & ~PGMASK;
                uint64_t page_offset = phdr.p_vaddr & PGMASK;
                uint32_t read_bytes, zero_bytes;
                if (phdr.p_filesz > 0) {
                    /* Normal segment.
						 * Read initial part from disk and zero the rest. */
                    read_bytes = page_offset + phdr.p_filesz;
                    zero_bytes = (ROUND_UP(page_offset + phdr.p_memsz, PGSIZE) -
                                  read_bytes);
                } else {
                    /* Entirely zero.
						 * Don't read anything from disk. */
                    read_bytes = 0;
                    zero_bytes = ROUND_UP(page_offset + phdr.p_memsz, PGSIZE);
                }
                if (!load_segment(file, file_page, (void *)mem_page, read_bytes,
                                  zero_bytes, writable)) {
                    goto done;
                }
            } else {
                goto done;
            }
            break;
        }
    }

    /* Set up stack. */
    if (!setup_stack(if_)) {
        goto done;
    }

    /* Start address. */
    if_->rip = ehdr.e_entry;

    /* 인자 문자열과 argv를 사용자 스택에 배치한다. */

    while (token != NULL) {
        /* 종료 문자까지 복사하며, 현재 스택 페이지를 벗어나지 않게 한다. */
        size_t bytes = strlen(token) + 1;
        if (argc >= arg_cnt || bytes > PGSIZE - (USER_STACK - if_->rsp)) {
            goto done;
        }
        /* 스택은 낮은 주소로 자란다. 복사 직후의 주소를 순서대로 기억한다. */
        if_->rsp -= bytes;
        strlcpy((char *)if_->rsp, token, bytes);
        arg_addrs[argc++] = (char *)if_->rsp;

        token = strtok_r(NULL, " ", &save_ptr);
    }

    /* 포인터를 놓기 전 스택 주소를 포인터 크기의 배수로 맞춘다. */
    size_t padding = if_->rsp % sizeof(void *);
    /* 인자 주소 argc개, argv 끝의 NULL, 가짜 반환 주소 한 칸이다. */
    size_t pointer_bytes = (argc + 2) * sizeof(void *);
    /* 한 페이지에서 문자열이 이미 차지한 공간을 제외한 나머지와 비교한다. */
    if (padding + pointer_bytes > PGSIZE - (USER_STACK - if_->rsp)) {
        goto done;
    }
    if_->rsp -= padding;
    /* 정렬을 위해 비워 둔 바이트를 0으로 채운다. 
    다만 이 코드는 kpage = palloc_get_page(PAL_USER | PAL_ZERO); 에서
    새 페이지 전체를 0으로 채우기 때문에 중복 작업이라 불필요한 코드이다.*/
    memset((void *)if_->rsp, 0, padding);

    /* argv[argc]는 인자 목록의 끝을 알리는 NULL이다. */
    if_->rsp -= sizeof(void *);
    *(char **)if_->rsp = NULL;
    /* 주소를 역순으로 쌓아 argv[0]이 가장 낮은 주소에 오게 한다. */
    for (i = argc - 1; i >= 0; i--) {
        if_->rsp -= sizeof(void *);
        *(char **)if_->rsp = arg_addrs[i];
    }
    /* 사용자 프로그램의 첫 두 인자: argc와 argv 배열의 시작 주소. */
    if_->R.rdi = argc;
    if_->R.rsi = if_->rsp;

    /* 함수 호출 직후처럼 보이도록 가짜 반환 주소 한 칸을 둔다. */
    if_->rsp -= sizeof(void *);
    *(void **)if_->rsp = NULL;

    success = true;

done:
    /* 성공과 실패 모두 이 지점에서 임시 자원을 정리한다. */
    /* 임시 주소 목록만 해제한다. 문자열은 사용자 스택에 별도로 남아 있다. */
    free(arg_addrs);
    file_close(file);
    return success;
}

/* Checks whether PHDR describes a valid, loadable segment in
 * FILE and returns true if so, false otherwise. */
static bool validate_segment(const struct Phdr *phdr, struct file *file)
{
    /* p_offset and p_vaddr must have the same page offset. */
    if ((phdr->p_offset & PGMASK) != (phdr->p_vaddr & PGMASK)) {
        return false;
    }

    /* p_offset must point within FILE. */
    if (phdr->p_offset > (uint64_t)file_length(file)) {
        return false;
    }

    /* p_memsz must be at least as big as p_filesz. */
    if (phdr->p_memsz < phdr->p_filesz) {
        return false;
    }

    /* The segment must not be empty. */
    if (phdr->p_memsz == 0) {
        return false;
    }

    /* The virtual memory region must both start and end within the
	   user address space range. */
    if (!is_user_vaddr((void *)phdr->p_vaddr)) {
        return false;
    }
    if (!is_user_vaddr((void *)(phdr->p_vaddr + phdr->p_memsz))) {
        return false;
    }

    /* The region cannot "wrap around" across the kernel virtual
	   address space. */
    if (phdr->p_vaddr + phdr->p_memsz < phdr->p_vaddr) {
        return false;
    }

    /* Disallow mapping page 0.
	   Not only is it a bad idea to map page 0, but if we allowed
	   it then user code that passed a null pointer to system calls
	   could quite likely panic the kernel by way of null pointer
	   assertions in memcpy(), etc. */
    if (phdr->p_vaddr < PGSIZE) {
        return false;
    }

    /* It's okay. */
    return true;
}

#ifndef VM
/* Codes of this block will be ONLY USED DURING project 2.
 * If you want to implement the function for whole project 2, implement it
 * outside of #ifndef macro. */

/* load() helpers. */
static bool install_page(void *upage, void *kpage, bool writable);

/* Loads a segment starting at offset OFS in FILE at address
 * UPAGE.  In total, READ_BYTES + ZERO_BYTES bytes of virtual
 * memory are initialized, as follows:
 *
 * - READ_BYTES bytes at UPAGE must be read from FILE
 * starting at offset OFS.
 *
 * - ZERO_BYTES bytes at UPAGE + READ_BYTES must be zeroed.
 *
 * The pages initialized by this function must be writable by the
 * user process if WRITABLE is true, read-only otherwise.
 *
 * Return true if successful, false if a memory allocation error
 * or disk read error occurs. */
static bool load_segment(struct file *file, off_t ofs, uint8_t *upage,
                         uint32_t read_bytes, uint32_t zero_bytes,
                         bool writable)
{
    ASSERT((read_bytes + zero_bytes) % PGSIZE == 0);
    ASSERT(pg_ofs(upage) == 0);
    ASSERT(ofs % PGSIZE == 0);

    file_seek(file, ofs);
    while (read_bytes > 0 || zero_bytes > 0) {
        /* Do calculate how to fill this page.
		 * We will read PAGE_READ_BYTES bytes from FILE
		 * and zero the final PAGE_ZERO_BYTES bytes. */
        size_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;
        size_t page_zero_bytes = PGSIZE - page_read_bytes;

        /* Get a page of memory. */
        uint8_t *kpage = palloc_get_page(PAL_USER);
        if (kpage == NULL) {
            return false;
        }

        /* Load this page. */
        if (file_read(file, kpage, page_read_bytes) != (int)page_read_bytes) {
            palloc_free_page(kpage);
            return false;
        }
        memset(kpage + page_read_bytes, 0, page_zero_bytes);

        /* Add the page to the process's address space. */
        if (!install_page(upage, kpage, writable)) {
            printf("fail\n");
            palloc_free_page(kpage);
            return false;
        }

        /* Advance. */
        read_bytes -= page_read_bytes;
        zero_bytes -= page_zero_bytes;
        upage += PGSIZE;
    }
    return true;
}

/* Create a minimal stack by mapping a zeroed page at the USER_STACK */
static bool setup_stack(struct intr_frame *if_)
{
    uint8_t *kpage;
    bool success = false;

    kpage = palloc_get_page(PAL_USER | PAL_ZERO);
    if (kpage != NULL) {
        success = install_page(((uint8_t *)USER_STACK) - PGSIZE, kpage, true);
        if (success) {
            if_->rsp = USER_STACK;
        } else {
            palloc_free_page(kpage);
        }
    }
    return success;
}

/* Adds a mapping from user virtual address UPAGE to kernel
 * virtual address KPAGE to the page table.
 * If WRITABLE is true, the user process may modify the page;
 * otherwise, it is read-only.
 * UPAGE must not already be mapped.
 * KPAGE should probably be a page obtained from the user pool
 * with palloc_get_page().
 * Returns true on success, false if UPAGE is already mapped or
 * if memory allocation fails. */
static bool install_page(void *upage, void *kpage, bool writable)
{
    struct thread *t = thread_current();

    /* Verify that there's not already a page at that virtual
	 * address, then map our page there. */
    return (pml4_get_page(t->pml4, upage) == NULL &&
            pml4_set_page(t->pml4, upage, kpage, writable));
}
#else
/* From here, codes will be used after project 3.
 * If you want to implement the function for only project 2, implement it on the
 * upper block. */

static bool lazy_load_segment(struct page *page, void *aux)
{
    /* TODO: Load the segment from the file */
    /* TODO: This called when the first page fault occurs on address VA. */
    /* TODO: VA is available when calling this function. */
}

/* Loads a segment starting at offset OFS in FILE at address
 * UPAGE.  In total, READ_BYTES + ZERO_BYTES bytes of virtual
 * memory are initialized, as follows:
 *
 * - READ_BYTES bytes at UPAGE must be read from FILE
 * starting at offset OFS.
 *
 * - ZERO_BYTES bytes at UPAGE + READ_BYTES must be zeroed.
 *
 * The pages initialized by this function must be writable by the
 * user process if WRITABLE is true, read-only otherwise.
 *
 * Return true if successful, false if a memory allocation error
 * or disk read error occurs. */
static bool load_segment(struct file *file, off_t ofs, uint8_t *upage,
                         uint32_t read_bytes, uint32_t zero_bytes,
                         bool writable)
{
    ASSERT((read_bytes + zero_bytes) % PGSIZE == 0);
    ASSERT(pg_ofs(upage) == 0);
    ASSERT(ofs % PGSIZE == 0);

    while (read_bytes > 0 || zero_bytes > 0) {
        /* Do calculate how to fill this page.
		 * We will read PAGE_READ_BYTES bytes from FILE
		 * and zero the final PAGE_ZERO_BYTES bytes. */
        size_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;
        size_t page_zero_bytes = PGSIZE - page_read_bytes;

        /* TODO: Set up aux to pass information to the lazy_load_segment. */
        void *aux = NULL;
        if (!vm_alloc_page_with_initializer(VM_ANON, upage, writable,
                                            lazy_load_segment, aux)) {
            return false;
        }

        /* Advance. */
        read_bytes -= page_read_bytes;
        zero_bytes -= page_zero_bytes;
        upage += PGSIZE;
    }
    return true;
}

/* Create a PAGE of stack at the USER_STACK. Return true on success. */
static bool setup_stack(struct intr_frame *if_)
{
    bool success = false;
    void *stack_bottom = (void *)(((uint8_t *)USER_STACK) - PGSIZE);

    /* TODO: Map the stack on stack_bottom and claim the page immediately.
	 * TODO: If success, set the rsp accordingly.
	 * TODO: You should mark the page is stack. */
    /* TODO: Your code goes here */

    return success;
}
#endif /* VM */
