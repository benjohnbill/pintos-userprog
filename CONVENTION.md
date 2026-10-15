# 코딩 컨벤션

Linux kernel 스타일을 기준으로 두 가지만 바꿨다.

1. 들여쓰기는 **스페이스 4칸**이다 (커널은 탭 8칸).
2. `if`, `for`, `while` 본문이 한 줄이어도 **중괄호를 항상** 쓴다.

## 도구가 맞춰 주는 것

| 규칙 | 예 | 도구 |
|---|---|---|
| 들여쓰기 4칸, 80열 | | 저장하면 자동 수정 (`.clang-format`) |
| 제어문 뒤 공백, 함수 호출에는 공백 없음 | `if (x)`, `foo(x)` | 저장하면 자동 수정 |
| 중괄호 위치: 함수는 다음 줄, 나머지는 같은 줄 | 아래 예시 | 저장하면 자동 수정 |
| 중괄호 항상 | `if (x) { return; }` | 저장하면 자동 수정 + 경고 (`.clang-tidy`) |
| 이름은 snake_case | `open_file`, `fd_count` | 노란 밑줄 경고 |
| 상수 매크로와 enum 값은 대문자 | `MAX_FD`, `THREAD_READY` | 노란 밑줄 경고 |

```c
static int find_fd(struct thread *t, int fd)
{
    if (fd < 0) {
        return -1;
    }
    return 0;
}
```

## 사람이 맞추는 것 (리뷰에서 본다)

- **함수 이름은 `모듈_동사`** 순서다. Pintos가 이미 이렇게 쓴다:
  `thread_create`, `list_push_back`, `palloc_get_page`.
  새 함수도 `process_exec`, `syscall_handler`처럼 모듈을 앞에 둔다.
- **같은 개념은 같은 이름**으로 부른다. 파일 디스크립터는 `fd`,
  사용자 주소는 `uaddr`처럼 한 번 정한 이름을 다른 파일에서도 쓴다.
- 매크로 함수는 소문자도 된다 (`list_entry`, `pg_ofs`). 섞인 대소문자(`maxLen`)는 안 된다.
- 이름 끝 `_`는 Pintos 관용구다: `void *buf_`를 받아 `uint8_t *buf = buf_`로 바꿔 쓴다.

## 명령

저장소 루트에서 실행한다.

```bash
make format   # 모든 C 파일을 컨벤션대로 고친다
make lint     # 위반을 출력한다. 위반이 있으면 종료 코드 1
```

`make lint`는 `threads/ devices/ userprog/ filesys/`의 이름 규칙을 본다.
`lib/`(C 표준 라이브러리 이름)와 `vm/`(Project 3)은 제외한다.

## 설정 (처음 한 번)

**VS Code + Dev Container**: `git pull` 후 VS Code가 띄우는
"Rebuild Container"를 누른다. 다른 설치는 필요 없다.
도구(clangd, clang-format, clang-tidy)와 clangd 확장, 저장 시 포맷이 이미 설정돼 있다.

**Dev Container 없이 쓰는 경우**: 같은 버전을 설치한다.
버전이 다르면 서로의 줄을 계속 다시 포맷하게 된다.

```bash
pip install clangd==21.1.8 clang-format==21.1.8 clang-tidy==21.1.6
```

에디터의 clangd도 이 `clangd`를 쓰게 지정한다.

## 전체 포맷 커밋

이 컨벤션을 처음 적용할 때 전체 파일을 한 번 포맷했다. 그 커밋은
`.git-blame-ignore-revs`에 있다. blame에서 숨기려면 한 번 실행한다:

```bash
git config blame.ignoreRevsFile .git-blame-ignore-revs
```

GitHub의 blame 화면은 이 파일을 자동으로 읽는다.
