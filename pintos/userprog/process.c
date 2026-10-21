#include "userprog/process.h"
#include <debug.h>
#include <inttypes.h>
#include <round.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "list.h"
#include "threads/synch.h"
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
#ifdef VM
#include "vm/vm.h"
#endif

static void process_cleanup(void);
/* const char *file_name -> char *file_name 로 변경 */
static bool load(char *file_name, struct intr_frame *if_);
static void initd(void *f_name);
static void __do_fork(void *);

static bool
extract_thread_name(const char *cmd_line, char *thread_name,
                    size_t size); // 실행 파일 이름만 별도로 추출하는 함수

/* General process initializer for initd and other process. */
static void process_init(void)
{
    struct thread *current = thread_current();
}

int process_add_file(struct file *file)
{
    struct thread *current = thread_current();
    struct fd_entry *fd_entry;

    if (file == NULL) {
        return -1;
    }

    fd_entry = malloc(sizeof *fd_entry);
    if (fd_entry == NULL) {
        return -1;
    }

    fd_entry->fd = current->next_fd;
    fd_entry->file = file;
    list_push_back(&current->fd_list, &fd_entry->elem);
    current->next_fd++;

    return fd_entry->fd;
}

struct file *process_find_file(int fd)
{
    struct thread *current = thread_current();
    struct list_elem *elem;
    struct fd_entry *fd_entry;

    elem = list_begin(&current->fd_list);

    while (elem != list_end(&current->fd_list)) {
        fd_entry = list_entry(elem, struct fd_entry, elem);
        if (fd_entry->fd == fd) {
            return fd_entry->file;
        }
        elem = list_next(elem);
    }
    return NULL;
}

bool process_close_file(int fd)
{
    struct thread *current = thread_current();
    struct list_elem *elem;
    struct fd_entry *fd_entry;

    elem = list_begin(&current->fd_list);

    while (elem != list_end(&current->fd_list)) {
        fd_entry = list_entry(elem, struct fd_entry, elem);
        if (fd_entry->fd == fd) {
            file_close(fd_entry->file);
            list_remove(&fd_entry->elem);
            free(fd_entry);

            return true;
        }
        elem = list_next(elem);
    }
    return false;
}

static void process_close_all_files(void)
{
    struct list_elem *elem;
    struct fd_entry *fd_entry;
    struct thread *current = thread_current();

    while (!list_empty(&current->fd_list)) {
        elem = list_pop_front(&current->fd_list);
        fd_entry = list_entry(elem, struct fd_entry, elem);
        file_close(fd_entry->file);
        free(fd_entry);
    }
}

/* Starts the first userland program, called "initd", loaded from FILE_NAME.
 * The new thread may be scheduled (and may even exit)
 * before process_create_initd() returns. Returns the initd's
 * thread id, or TID_ERROR if the thread cannot be created.
 * Notice that THIS SHOULD BE CALLED ONCE. */
tid_t process_create_initd(const char *file_name)
{
    struct process_start_info *start_info;
    struct child_status *child_status;
    char *fn_copy;        // initd/load에 전달할 전체 명령줄 복사본
    char thread_name[16]; // 실행 파일의 이름 저장할 배열
    tid_t tid;

    if (!extract_thread_name(file_name, thread_name, sizeof(thread_name))) {
        return TID_ERROR;
    }

    /* start_info 동적 할당 */
    start_info = malloc(sizeof *start_info);
    if (start_info == NULL) {
        return TID_ERROR;
    }

    /* Make a copy of FILE_NAME.
	 * Otherwise there's a race between the caller and load(). */
    fn_copy = palloc_get_page(0);
    if (fn_copy == NULL) {
        free(start_info);
        return TID_ERROR;
    }

    /* child_status 동적 할당 */
    child_status = malloc(sizeof *child_status);
    if (child_status == NULL) {
        free(start_info);
        palloc_free_page(fn_copy);
        return TID_ERROR;
    }

    /* child_status 초기화 */
    child_status->status = -1;
    child_status->is_waited = false;
    sema_init(&child_status->exit_sema, 0);

    strlcpy(fn_copy, file_name, PGSIZE);

    start_info->file_name = fn_copy;
    start_info->child_status = child_status;

    /* 해야할 일:
     1. child_status 동적 할당 v
     2. status, is_waited, exit_sema 초기화 v
     3. tread_create() v
     4. 생성된 tid 기록 v
     5. 부모의 children 목록에 등록 v
     
     현재 thread_create()은 fn_copy만 전달하고 있는데 이제 child_status도 전달해야 함.
     따라서 두 정보를 묶는 구조체가 하나 더 필요...*/

    /* Create a new thread to execute FILE_NAME. */
    tid = thread_create(thread_name, PRI_DEFAULT, initd, start_info);
    if (tid == TID_ERROR) {
        palloc_free_page(fn_copy);
        free(start_info);
        free(child_status);
        return TID_ERROR;
    }

    child_status->tid = tid;
    list_push_back((&thread_current()->children), &child_status->elem);
    return tid;
}

/* 전체 명령줄에서 실행 파일 일므인 첫 번째 토큰 추출하는 함수 */
static bool extract_thread_name(const char *cmd_line, char *thread_name,
                                size_t size)
{
    size_t cmd_idx = 0;  // cmd_line에서 현재 읽고 있는 위치
    size_t name_idx = 0; // thread_name에 다음 문자를 저장할 위치

    if (size == 0) {
        return false;
    }

    while (cmd_line[cmd_idx] == ' ') {
        cmd_idx++;
    }
    if (cmd_line[cmd_idx] == '\0') {
        return false;
    }

    while (name_idx + 1 < size && cmd_line[cmd_idx] != ' ' &&
           cmd_line[cmd_idx] != '\0') {
        thread_name[name_idx] = cmd_line[cmd_idx];
        name_idx++;
        cmd_idx++;
    }

    thread_name[name_idx] = '\0';
    return true;
}

/* A thread function that launches first user process. */
/* problem to be solved:
 thread_create()에 start_info()를 넘기는데, 현재 initd()는 여전히 받은 void *를 명령줄인 f_name으로 간주하여 그대로
 process_exec()에 전달 따라서
 1. void *를 struct process_start_info *로 해석 v
 2. 현재 자식 스레드의 child_status 포인터를 연결 v
 3. file_name을 별도로 꺼냄 v
 4. 전달용 start_info만 해제 v
 5. 꺼낸 file_name을 process_exec()에 전달 */
static void initd(void *aux_)
{
#ifdef VM
    supplemental_page_table_init(&thread_current()->spt);
#endif
    struct process_start_info *start_info = aux_;
    char *file_name = start_info->file_name;

    thread_current()->child_status = start_info->child_status;
    free(start_info);

    process_init();

    if (process_exec(file_name) < 0) {
        PANIC("Fail to launch initd\n");
    }
    NOT_REACHED();
}

/* Clones the current process as `name`. Returns the new process's thread id, or
 * TID_ERROR if the thread cannot be created. */
/* 부모의 실행 상태와 자식 상태를 준비한 뒤 자식 스레드를 생성한다
   부모는 자식의 주소 공간 복제가 끝날 때까지 기다리고,
   성공하면 자식 tid를 반환하고 실패하면 TID_ERROR를 반환한다.
   ********************************************************
   부모 쪽에서 fork를 준비하는 함수
   로직:
   1. fork_start_info와 child_status 할당
   2. child_status 초기화
   3. 부모 스레드와 intr_frame 저장
   4. fork 완료용 fork_sema 초기화
   5. thread_create()로 자식 생성
   6. 자식의 주소 공간 복제가 끝날 때까지 sema_down()
   7. 성공하면 부모의 children 목록에 자식 등록
   8. 부모에게 자식 tid 반환
   9. 실패하면 자식 종료 후 child_status 해제 */

tid_t process_fork(const char *name, struct intr_frame *if_)
{
    struct fork_start_info *start_info;
    tid_t tid;
    bool is_forked;
    struct child_status *child_status;

    start_info = malloc(sizeof *start_info);
    if (start_info == NULL) {
        return TID_ERROR;
    }

    child_status = malloc(sizeof *child_status);
    if (child_status == NULL) {
        free(start_info);
        return TID_ERROR;
    }

    start_info->parent = thread_current();
    memcpy(&start_info->parent_if, if_, sizeof(start_info->parent_if));

    sema_init(&start_info->fork_sema, 0);
    start_info->is_forked = false;

    child_status->status = -1;
    child_status->is_waited = false;
    sema_init(&child_status->exit_sema, 0);

    start_info->child_status = child_status;

    /* Clone current thread to new thread.*/
    tid = thread_create(name, PRI_DEFAULT, __do_fork, start_info);
    if (tid == TID_ERROR) {
        free(start_info);
        free(child_status);
        return TID_ERROR;
    }

    child_status->tid = tid;

    sema_down(&start_info->fork_sema);

    is_forked = start_info->is_forked;
    free(start_info);

    if (is_forked == false) {
        sema_down(&child_status->exit_sema);
        free(child_status);
        return TID_ERROR;
    }

    list_push_back((&thread_current()->children), &child_status->elem);
    return tid;
}

#ifndef VM
/* Duplicate the parent's address space by passing this function to the
 * pml4_for_each. This is only for the project 2. */

/* 부모의 사용자 페이지 하나를 자식 주소 공간에 복제한다.
   페이지 내용뿐 아니라 부모 매핑의 쓰기 권한도 동일하게 보존한다.
   ********************************************************
   pml4_for_each()가 부모의 각 페이지 마다 호출
   로직:
   1. 커널 가상 주소면 복제하지 않고 건너 뜀
   2. 부모 페이지 테이블에서 물리 페이지 조회
   3. 자식용 사용자 페이지 할당
   4. PGSIZE만큼 원시 메모리 복사
   5. 부모 PTE의 쓰기 권한 확인
   6. 자식 페이지 테이블에 같은 가상 주소로 매핑
   7. 매핑 실패 시 할당한 페이지 해제 */
static bool duplicate_pte(uint64_t *pte, void *va, void *aux)
{
    struct thread *current = thread_current();
    struct thread *parent = (struct thread *)aux;
    void *parent_page;
    void *newpage;
    bool writable;

    /* 1. TODO: If the parent_page is kernel page, then return immediately. */
    if (is_kernel_vaddr(va)) {
        return true;
    }
    /* 2. Resolve VA from the parent's page map level 4. */
    parent_page = pml4_get_page(parent->pml4, va);

    /* 3. TODO: Allocate new PAL_USER page for the child and set result to
	 *    TODO: NEWPAGE. */
    newpage = palloc_get_page(PAL_USER);
    if (newpage == NULL) {
        return false;
    }

    /* 4. TODO: Duplicate parent's page to the new page and
	 *    TODO: check whether parent's page is writable or not (set WRITABLE
	 *    TODO: according to the result). */
    memcpy(newpage, parent_page, PGSIZE);
    writable = is_writable(pte);
    /* 5. Add new page to child's page table at address VA with WRITABLE
	 *    permission. */
    if (!pml4_set_page(current->pml4, va, newpage, writable)) {
        /* 6. TODO: if fail to insert page, do error handling. */
        palloc_free_page(newpage);
        return false;
    }
    return true;
}
#endif

/* A thread function that copies parent's execution context.
 * Hint) parent->tf does not hold the userland context of the process.
 *       That is, you are required to pass second argument of process_fork to
 *       this function. */
/* 새 자식 스레드에서 부모의 실행 문맥과 주소 공간을 복제한다.
   복제가 끝나면 결과를 부모에게 알리고 사용자 영역으로 진입
   ****************************************************
   새로 생성된 자식 스레드에서 실행되는 함수
   로직:
   1. 부모의 intr_frame 복사
   2. 자식의 fork 반환값인 RAX 0으로 설정
   3. 자식용 페이지 테이블 생성
   4. 부모 주소 공간 복제
   5. 복제 결과를 is_forked에 기록
   6. sema_up으로 부모 깨움
   7. 성공하면 do_iret()으로 자식 사용자 코드에 복귀 */
static void __do_fork(void *aux)
{
    struct intr_frame if_;
    // struct thread *parent = (struct thread *)aux;
    struct thread *current = thread_current();
    /* TODO: somehow pass the parent_if. (i.e. process_fork()'s if_) */
    struct fork_start_info *start_info = aux;
    struct thread *parent = start_info->parent;
    struct intr_frame *parent_if = &start_info->parent_if;
    struct list_elem *fd_elem;        /* 부모의 fd 목록을 순회할 요소 */
    struct fd_entry *parent_fd_entry; /* 부모의 fd 항목 */
    struct fd_entry *child_fd_entry;  /* 자식의 fd 항목 */
    // bool succ = true;

    /* 1. Read the cpu context to local stack. */
    memcpy(&if_, parent_if, sizeof(struct intr_frame));

    /* fork()가 성공했을 때 반환값으로
    부모: 자식의 tid
    자식: 0
    자식이 사용자 영역으로 돌아갈 떄 복원할 레지스터 상태 이므로, 자식이 fork()의 반환값으로 0을 받도록 RAX를 0으로 설정 */
    if_.R.rax = 0;

    /* 페이지 복제 도중 실패하더라도 process_exit()이
       부모에게 종료 상태를 전달할 수 있도록 먼저 연결한다. */
    current->child_status = start_info->child_status;

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
    /* fd 복제의 로직:
       1. 부모의 fd_list를 순회
       2. 각 부모의 fd_entry마다 자식용 fd_entry를 malloc()
       3. 부모의 file을 file_duplicate()로 복제
       4. 자식 엔트리에 부모와 동일한 fd 번호 저장
       5. 완성된 엔트리를 current->fd_list에 저장
       6. 부모의 next_fd도 자식에게 복사
       7. 복제 실패 시 지금까지 만든 파일은 닫고 fd 항목 free하고 goto error
       8. 전부 성공한 후에만 is_forked = true, sema_up()실행 v */

    fd_elem = list_begin(&parent->fd_list);
    while (fd_elem != list_end(&parent->fd_list)) {
        parent_fd_entry = list_entry(fd_elem, struct fd_entry, elem);

        child_fd_entry = malloc(sizeof *child_fd_entry);
        if (child_fd_entry == NULL) {
            goto error;
        }

        child_fd_entry->file = file_duplicate(parent_fd_entry->file);
        if (child_fd_entry->file == NULL) {
            free(child_fd_entry);
            goto error;
        }
        child_fd_entry->fd = parent_fd_entry->fd;
        list_push_back(&current->fd_list, &child_fd_entry->elem);
        fd_elem = list_next(fd_elem);
    }
    current->next_fd = parent->next_fd;

    process_init();

    start_info->is_forked = true;
    sema_up(&start_info->fork_sema);
    /* Finally, switch to the newly created process. */
    do_iret(&if_);
    // if (succ) {
    //     do_iret(&if_);
    // }

/* 부모에게 복제 실패를 전달하려면 공유 상태와 동기화가 필요
   1. 자식이 startr_info->is_forked = false로 기록
   2. fork_sema를 올려 기다리던 부모를 깨움
   3. 부모가 이를 확인하고 TID_ERROR, 즉 -1 반환
   4. 자식은 thread_exit()으로 종료 */
error:
    start_info->is_forked = false;
    sema_up(&start_info->fork_sema);
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
 * This function will be implemented in problem 2-2.  For now, it
 * does nothing. */

/* CHILD_TID가 현재 프로세스의 자식인지 확인하고
   아직 종료하지 않았다면 종료할 떄까지 기다린다.
   종료 상태를 회수한 뒤 자식 상태를 목록에서 제거하고 해제한다.
   유효한 자식이 아니거나 이미 기다린 자식이면 -1을 반환한다.
   *******************************************************
   로직:
   1. 부모의 children 목록을 순회하면서 전달받은 child_tid와 같은 상태 객체를 찾는다.
   2. 목록 끝까지 찾지 못하면 존재하지 않는 pid, 다른 프로세스의 자식, 이미 wait하여 목록에서 제거한 자식
   이므로 -1 반환.
   3. 하나의 자식에 대한 종료 상태는 한 번만 회수 하므로 중복 wait을 검사한다.
   4. 자식이 실행 중이면 부모가 잠들고, 자식이 이미 종료했다면 저장된 세마포어 신호를 소비하고 즉시 통과한다.
   5. 부모가 꺠어나면 자식은 더 이상 child_status를 사용하지 않으므로 free하고 status 반환
  */
int process_wait(tid_t child_tid)
{
    /* XXX: Hint) The pintos exit if process_wait (initd), we recommend you
	 * XXX:       to add infinite loop here before
	 * XXX:       implementing the process_wait. */
    struct list *children = &thread_current()->children;
    struct list_elem *e = list_begin(children);

    struct child_status *child_status = NULL;
    int status;

    while (e != list_end(children)) {
        child_status = list_entry(e, struct child_status, elem);
        if (child_status->tid == child_tid) {
            break;
        }
        e = list_next(e);
    }

    if (e == list_end(children)) {
        return -1;
    }

    if (child_status->is_waited == false) {
        child_status->is_waited = true;
        sema_down(&child_status->exit_sema);
    } else {
        return -1;
    }
    status = child_status->status;

    list_remove(&child_status->elem);
    free(child_status);
    return status;
}

/* process_exit의 흐름
 1. 현재 스레드의 종료 메시지 출력
 2. child_status가 있는지 확인
 3. curr->exit_status를 child_status->status에 복사
 4. 현재 프로세스가 보유한 모든 파일 디스크립터 정리
 5. 프로세스 주소 공간 정리
 6. exit_sema를 sema_up()해서 부모 깨우기 */
/* Exit the process. This function is called by thread_exit (). */
void process_exit(void)
{
    struct thread *curr = thread_current();

    /* Kernel threads have no page table; only user processes print. */
    if (curr->pml4 != NULL) {
        printf("%s: exit(%d)\n", curr->name, curr->exit_status);
    }

    /* TODO: Your code goes here.
	 * TODO: Implement process termination message (see
	 * TODO: project2/process_termination.html).
	 * TODO: We recommend you to implement process resource cleanup here. */

    if (curr->child_status != NULL) {
        curr->child_status->status = curr->exit_status;
    }
    process_close_all_files();
    process_cleanup();

    if (curr->child_status != NULL) {
        sema_up(&curr->child_status->exit_sema);
    }
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
