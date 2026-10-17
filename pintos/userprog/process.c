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
#include "threads/palloc.h"
#include "threads/thread.h"
#include "threads/mmu.h"
#include "threads/vaddr.h"
#include "intrinsic.h"
#ifdef VM
#include "vm/vm.h"
#endif

static void process_cleanup(void);
static bool load(const char *file_name, struct intr_frame *if_);
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
tid_t process_create_initd(const char *file_name)
{
    char *fn_copy;
    tid_t tid;

    /* Make a copy of FILE_NAME.
	 * Otherwise there's a race between the caller and load(). */
    fn_copy = palloc_get_page(0);
    if (fn_copy == NULL) {
        return TID_ERROR;
    }
    strlcpy(fn_copy, file_name, PGSIZE);

    /* Create a new thread to execute FILE_NAME. */
    tid = thread_create(file_name, PRI_DEFAULT, initd, fn_copy);
    if (tid == TID_ERROR) {
        palloc_free_page(fn_copy);
    }
    return tid;
}

/* A thread function that launches first user process. */
static void initd(void *f_name)
{
#ifdef VM
    supplemental_page_table_init(&thread_current()->spt);
#endif

    process_init();

    if (process_exec(f_name) < 0) {
        PANIC("Fail to launch initd\n");
    }
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
/* argument passing을 위한 process_exec()의 로직:
 * f_name을 전체 명령줄로 보존
 * 파싱용 복사본 할당 및 복사
 * 복사본에서 실행 파일 이름 추출
 * 현재 프로세스 주소 공간 정리
 * load(전체 명령줄, &_if)
 * 복사본과 원본 정리
 * 성공하면 do_iret() */
int process_exec(void *f_name)
{
    char *file_name = f_name;

    /* load에 미리 쪼개서 주면 편하다고 생각해서 만든 변수들인데 그럴 이유가 없었음. */
    // char *cmd_cpy;            // 파싱용 복사본
    // char *exec_name;          // 복사본에서 얻을첫 번째 토큰
    // char *save_ptr;           // strtok_r()의 파싱 위치
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
    /* 의견) 변수 이름 졸라 헷갈리는데 뭐라고 다들 하셨는지 */
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
 * This function will be implemented in problem 2-2.  For now, it
 * does nothing. */
int process_wait(tid_t child_tid UNUSED)
{
    /* XXX: Hint) The pintos exit if process_wait (initd), we recommend you
	 * XXX:       to add infinite loop here before
	 * XXX:       implementing the process_wait. */
    return -1;
}

/* Exit the process. This function is called by thread_exit (). */
void process_exit(void)
{
    struct thread *curr = thread_current();
    /* TODO: Your code goes here.
	 * TODO: Implement process termination message (see
	 * TODO: project2/process_termination.html).
	 * TODO: We recommend you to implement process resource cleanup here. */

    process_cleanup();
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

/* Loads an ELF executable from FILE_NAME into the current thread.
 * Stores the executable's entry point into *RIP
 * and its initial stack pointer into *RSP.
 * Returns true if successful, false otherwise. */

/* load()의 역할 및 TODO << 10.16일 작성 주석 삽질임.
 * 실행 파일 이름 확보 <- file_name
 * 분리된 명령줄도 확보 <- 이건 매개변수로 받고 있지 않기 때문에 추가해야 함 const char* cmd_line (setup_stack()도 이 인자 받아야 함)
 * => process_exec()에서 실행 파일 이름을 분리하는 과정 필요함!! >> 필요 없어 바보야
 * ELF 로딩 <- 이건 이미 작성 되어 있음
 * argument passing
 * register 설정 */

/* load 함수의 로직:
 1. cmd_cpy 할당
 2. file_name 복사
 3. exec_name 추출
 4. exec_name이 NULL인지 검사
 5. filesys_open(exec_name) << 지금은 file_name으로 들어가 있음
 6. ELF 로딩 << already done
 7. setup_stack() << dont need to modify
 8. TODO 이후 argument passing */
static bool load(const char *file_name, struct intr_frame *if_)
{
    struct thread *t = thread_current();
    struct ELF ehdr;
    struct file *file = NULL;
    off_t file_ofs;
    bool success = false;
    int i;
    char *cmd_cpy = NULL; // 파싱된 문자들이 실제로 저장된 버퍼
    char *save_ptr;       // 다음 토큰을 찾을 위치
    char *exec_name;      // 복사본에서 얻을 첫번째 토큰

    /* cmd_cpy 메모리 할당 */
    cmd_cpy = palloc_get_page(0);
    if (cmd_cpy == NULL) {
        goto done;
    }

    /* file_name 복사 */
    strlcpy(cmd_cpy, file_name, PGSIZE);

    /* exec_name 추출 */
    exec_name = strtok_r(cmd_cpy, " ", &save_ptr);

    if (exec_name == NULL) {
        // palloc_free_page(file_name);
        //palloc_free_page(cmd_cpy);
        goto done;
    }

    /* Allocate and activate page directory. */
    t->pml4 = pml4_create();
    if (t->pml4 == NULL) {
        goto done;
    }
    process_activate(thread_current());

    /* Open executable file. */
    file = filesys_open(exec_name);
    if (file == NULL) {
        printf("load: %s: open failed\n", exec_name);
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
    /* 빈 스택 준비하기. argument passing은 이후에 한다. */
    if (!setup_stack(if_)) {
        goto done;
    }

    /* Start address. */
    if_->rip = ehdr.e_entry;

    /* TODO: Your code goes here.
	 * TODO: Implement argument passing (see project2/argument_passing.html). */

    char *token;         // 현재 얻은 토큰의 시작 주소
    int argc = 0;        // 위에서 exec_name으로 하나 쪼갰으니 1부터 시작
    char *argv[65];      // cmd_cpy 내부 토큰의 커널 주소를 저장하는 임시 배열
    char *user_argv[65]; // 사용자 스택에 복사된 문자열의 주소를 저장하는 배열

    /* argv와 user_argv로 나눈 이유
     * argv: cmd_cpy 안의 각 토큰을 가리키는 포인터 배열 -> 커널에서 사용하는 임시 배열임
     * user_argv: 사용자 스택에 복사한 뒤 얻은 사용자 주소 배열 
     * 원본 토큰 주소랑 복사된 사용자 주소 혼동 안하려고 두 단계로 나눔 */

    argv[0] = exec_name;
    argc++;

    token = strtok_r(NULL, " ", &save_ptr);

    /* 토큰을 argv라는 임시 배열에 반복문으로 저장 */
    while (token != NULL && argc < 64) {
        argv[argc] = token;
        argc++;
        token = strtok_r(NULL, " ", &save_ptr);
    }

    /* 최대 인자 개수 이상인지 검사 */
    if (token != NULL) {
        goto done;
    }

    argv[argc] = NULL;

    /* 각 토큰 문자열을 사용자 스택에 복사 */
    for (int i = argc - 1; i >= 0; i--) {
        if_->rsp -= strlen(argv[i]) + 1;
        memcpy((void *)if_->rsp, argv[i], strlen(argv[i]) + 1);
        user_argv[i] = (char *)if_->rsp;
    }

    /* 8배수 정렬 패딩 작업 */
    if (if_->rsp % 8 != 0) {
        if_->rsp -= if_->rsp % 8;
    }

    /* 사용자 스택에 목록 끝을 표기하기 위한 NULL 값 배치 */
    if_->rsp -= sizeof(char *);
    *(char **)if_->rsp = NULL;

    /* 문자열들의 주소를 역순으로 스택에 배치 */
    for (int i = argc - 1; i >= 0; i--) {
        if_->rsp -= sizeof(char *);
        *(char **)if_->rsp = user_argv[i];
    }

    /* 사용자 프로그램에 argc와 argv(user_argv)를 전달하기 위해 레지스터에 저장 */
    if_->R.rsi = if_->rsp;
    if_->R.rdi = argc;

    /* fake address 설정 */
    if_->rsp -= sizeof(char *);
    *(char **)if_->rsp = 0;

    success = true;

done:
    /* We arrive here whether the load is successful or not. */
    file_close(file);
    if (cmd_cpy != NULL) {
        palloc_free_page(cmd_cpy);
    }
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
/* setup_stack의 로직
   1. 스택 페이지 할당 & 매핑
   2. 명령줄 복사
   3. 토큰 파싱
   4. 문자열 역순 복사
   5. 8바이트 정렬
   6. argv[argc] 포인터 배열 배치
   7. RDI = argc, RSI = argv
   8. fake return address 배치
   9. cmd_copy 해제 */

/* 일어날 수 있는 오류 가능성
   1. token != NULL로 인자 초과 시 cmd_copy만 해제하고 매핑된 스택 페이지는 정리하지 않음
   2. cmd_opy 할당 이후 kpage가 실패하는 경로는 정리되어 있지만, 설치 성공 후 이후 단계에서 실패하는 경로의 정리는 더 필요함
   3. argv[65]는 임의 값 설정이므로 실제 초과 검사를 유지해야 함
   4. process_exec() 쪽의 cmd_cpy와 setup_stack()의 cmd_copy는 서로 다른 임시 페이지이므로 각각 해제되어야 함*/

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

// static bool setup_stack(const char *cmd_line, struct intr_frame *if_)
// {
//     uint8_t *kpage;
//     char *cmd_copy;      // 커널에서 strtok_r()로 파싱할 임시 버퍼
//     char *save_ptr;      // 다음 토큰을 찾을 위치
//     char *token;         // 현재 얻은 토큰의 시작 주소
//     int argc = 0;        // argument count
//     char *argv[65];      // 0~63개 실제 인자 + 마지막 NULL 값 임의로 정한 값임.
//     char *user_argv[65]; // 사용자 스택에 복사된 문자열 주소
//     bool success = false;

//     cmd_copy = palloc_get_page(0);
//     if (cmd_copy == NULL) {
//         return success;
//     }

//     /* kpage는 사용자 스택으로 매핑할 페이지*/

//     kpage = palloc_get_page(PAL_USER | PAL_ZERO);
//     if (kpage != NULL) {
//         success = install_page(((uint8_t *)USER_STACK) - PGSIZE, kpage, true);
//         if (success) {
//             if_->rsp =
//                 USER_STACK; // 사용자 스택의 시작 위치를 CPU의 RSP 레지스터 값으로 기록
//             strlcpy(cmd_copy, cmd_line, PGSIZE);
//         } else {
//             palloc_free_page(kpage);
//             palloc_free_page(cmd_copy);
//             return success;
//         }
//     } else {
//         palloc_free_page(cmd_copy);
//         return success;
//     }

//     /* 위 조건문을 모두 통과한 경우에만 파싱 진행 */
//     token = strtok_r(cmd_copy, " ", &save_ptr);

//     /* 토큰을 임시 배열에 저장하는 반복문 */
//     while (token != NULL && argc < 64) {
//         argv[argc] = token;
//         argc++;
//         token = strtok_r(NULL, " ", &save_ptr);
//     }

//     /* 최대 인자 개수(64개)를 초과하면 실패 처리 */
//     if (token != NULL) {
//         palloc_free_page(cmd_copy);
//         return false;
//     }
//     /* kpage 나중에 정리 필요 process_cleanup() */

//     argv[argc] = NULL;

//     /* 각 토큰 문자열을 사용자 스택에 복사하는 과정 */
//     for (int i = argc - 1; i >= 0; i--) {
//         if_->rsp -= strlen(argv[i]) + 1;
//         memcpy((void *)if_->rsp, argv[i], strlen(argv[i]) + 1);
//         user_argv[i] = (char *)if_->rsp;
//     }

//     /* 8배수 정렬을 위한 패딩 추가 작업 */
//     if (if_->rsp % 8 != 0) {
//         if_->rsp -= if_->rsp % 8;
//     }

//     /* 사용자 스택에 NULL 포인터를 배치. 인자 목록의 끝을 표시하기 위함 */
//     if_->rsp -= sizeof(char *);
//     *(char **)if_->rsp = NULL;

//     /* 사용자 스택에 복사한 문자열들의 주소를 역순으로 스택에 배치*/
//     for (int i = argc - 1; i >= 0; i--) {
//         if_->rsp -= sizeof(char *);
//         *(char **)if_->rsp = user_argv[i];
//     }

//     /* 사용자 프로그램에 argc와 argv를 전달하기 위해 레지스터 설정 */
//     if_->R.rsi = if_->rsp;
//     if_->R.rdi = argc;

//     /* fake return address 삽입 */
//     if_->rsp -= sizeof(char *);
//     *(char **)if_->rsp = 0;

//     palloc_free_page(cmd_copy);

//     return success;
// }

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
