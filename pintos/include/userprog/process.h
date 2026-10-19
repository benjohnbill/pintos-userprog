#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"
#include "threads/synch.h"

tid_t process_create_initd(const char *file_name);
tid_t process_fork(const char *name, struct intr_frame *if_);
int process_exec(void *f_name);
int process_wait(tid_t);
void process_exit(void);
void process_activate(struct thread *next);


struct child_status {
    tid_t tid; /* 어느 자식의 정보인지 */
    int status; /* 자식의 종료 상태 */
    bool is_waited; /* 이미 process_wait()가 호출 되었는지 */
    struct semaphore exit_sema; /* 자식 종료까지 부모를 재울 동기화 객체 */
    struct list_elem elem; /* 부모의 children 목록에 연결할 요소 */
};

struct process_start_info {
    char *file_name;                   // thread_create 함수에 전달할 file_name
    struct child_status *child_status; // thread_create 함수에 전달할 자식 정보
};

/* 부모의 실행 상태를 자식에게 전달하고 주소 공간 복제가 끝날 때까지 부모와 자식을 동기화
   child_status는 fork 이후 wait()/exit()에서 계속 사용하고,
   fork_start_info 자체는 fork 준비가 끝나면 부모가 해제한다. */
struct fork_start_info {
    struct thread *parent;       //부모의 페이지 테이블을 참조하기 위한 포인터
    struct intr_frame parent_if; // 시스템 콜 당시 부모의 레지스터 상태 복사본

    struct semaphore
        fork_sema; // 자식의 복제가 끝날 떄까지 부모를 재우는 세마포어
    bool is_forked;

    struct child_status *child_status; // wait()과 exit()이 공유할 자식 상태
};

#endif /* userprog/process.h */
