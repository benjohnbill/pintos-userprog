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

#endif /* userprog/process.h */
