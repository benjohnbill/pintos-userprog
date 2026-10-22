# A — Day-1 plan, 공통/분할 method, coding convention (fact sheet)

Researched 2026-10-22. Times are KST. Transcript timestamps are UTC in the files; converted here (+9h).
Days: 10-15 Thu (day 1) · 10-16 Fri · 10-17 Sat · 10-18 Sun · 10-19 Mon · 10-20 Tue · 10-21 Wed.

Sources used: git (week08 all branches, benjohnbill.github.io, week07), `gh pr view`, claude-mem, transcripts under
`~/.claude/projects/-home-benjohnbill-dev-krafton-jungle{,-week08}/`, whiteboard photo `IMG_8052.jpg`,
plan-page screenshots `cc-paste/20261022-0224*.png`.

## 1. Day-1 whiteboard

| Claim | Verdict | Evidence |
|---|---|---|
| Whiteboard is from day 1 (10-15) | **Unknown (plausible)** | Photo has no EXIF date (file copied 10-22 02:27). Consistent with: repo baseline imported 10-15 19:27 (`7f88707`), convention built the same evening. |
| Team activity unit = 회의 and PR, set by 작업 | **Confirmed** | Photo: `회의 / PR → 작업 → 공통 / 분할`. |
| 분할→공통 when logic/variables shared; everyone writes, then code review | **Confirmed** | Photo: `① 로직 ② 변수` between 공통 and 분할; also `함수/로직/API/변수 공유도` line. claude-mem #39563 (recorded 10-22 from his account, not from day 1). |
| 공통→분할 when time short; AI splits by test cases | **Confirmed** | Photo: `AI → Test Case` arrow into 분할. |
| fork/wait/exec = 공통 | **Confirmed** | Photo: `* fork() wait() execute()` under the to-do list. |
| Prerequisites args-passing, write/exit/wait (infinite loop) | **Not on the board; confirmed by history** | Not legible on photo. PR #105 (10-17 22:02) merged args + write/exit with `process_wait` still a `thread_yield` loop ("wait 구현 전 임시"). |
| Day-1 to-do: gitbook, Shell/userprog with AI, syscalls | **Confirmed** | Photo `<오늘 할일>`: `1. Pintos Doc` · `2. Shell이 어떻게 돌아가는지 (flag) (Userprog)` · `3. System Call`. "with AI" is not written on the board. |
| Convention block: 목표 · 현황 · 방법 체크 · CONVENTION.md | **Confirmed (wording differs)** | Photo under `Convention`: `목표 ✓`, `현황(test) ✓`, `방법론 체크 ✓`. |
| "Lint.md 기반으로 검색해줘", "3시간 이상" | **Confirmed on board; meaning unknown** | Photo bottom: `3시간 이상`, `Lint.md ⇒ "Lint.md" 기반으로 검색해줘`, `문서`. No file `Lint.md` exists in any repo; the document became `CONVENTION.md`. No source explains "3시간 이상". The convention session itself took ~2 h (19:43–21:41). |
| Extra board items he did not mention | — | `제3자 오픈소스 규약 → 검사기 → make check` (the lint-tool idea, later `make lint`); red register sketch `rdi rsi rdx r10 r8 r9`, User/Kernel, `syscall(…)` (syscall calling convention). |

## 2. 공통/분할 → the plan page ("3. 나눈 기준")

| Claim | Verdict | Evidence |
|---|---|---|
| 각자 = fork·wait·exec; three calls share "what a parent knows about its child" | **Confirmed** | `benjohnbill.github.io/study/week08/team-plan.html` § 3: "세 콜이 '부모가 자식에 대해 아는 것' 하나를 같이 씀 → 결합 최강 → 셋 다 직접 겪을 가치 최대". |
| 분업 = file syscalls | **Confirmed** | Same: "fork 없이 통과 → 순서 자유. 중요도가 낮다는 뜻은 아님. fd 테이블이 정해지면 핸들러 모양이 반복". |
| 이음매 = fd copy · rox | **Confirmed** | Same: "프로세스 쪽과 파일 쪽이 만나는 곳 → 팀원이 fd 테이블을 직접 다루는 자리". `team-plan.json`: tue-fd → Hannuri-Song, tue-rox → JSZeroDev. |
| 주말 선구현 = no push until Monday comparison | **Confirmed as plan; partly held** | Plan: "월요일 저녁 비교 전까지 push 없음. 팀원 구현이 막혀도 마감을 지킬 베이스". Reality: no fork/wait/exec commits by benjohnbill on 10-18 (Sun); `6fcb968 Complete wait()` is Mon 10-19 17:05, pushed 17:05 (`origin/benjohnbill` reflog). So the "weekend" base was written Monday; no push happened before Monday. |
| Page subtitle (not mentioned) | **Extra** | "가른 기준은 결합도와 fork 의존 여부" — coupling and fork dependence, i.e. the criterion is test dependency, not the whiteboard's logic/variable sharing. |

**This page is NOT day-1 material.** It came from a Claude grilling session on **Sat 10-17, 22:46–23:26 KST**
(transcript `545787f1-…jsonl`; claude-mem #38539 test-dependency count, #38558 page created 23:29).
How the split was reached, in order:
1. AI counted test dependencies: file tests (27) need no fork; wait tests all call fork, `wait-simple` also exec (#38539).
2. AI's first proposal: 각자 = pointer validation + process bundle; 분업 = fd table, file syscalls, rox.
3. He said only he could work the weekend → AI proposed his single process base, teammates learn by review.
4. He objected (23:15): "fork/exec/wait이 가장 중요한 쪽 아냐? … 파일이 fork하고 독립적이라고? 그럼 file이 중요도가 적어?" → AI: "독립은 순서 이야기이지 중요도 이야기가 아니에요" (wording kept on the page).
5. He proposed (23:16): "내가 3개 syscall을 미리 다 짜고, 월요일에 파일쪽 syscall을 짜도 상관없지 않나?" → AI accepted, added the no-push condition because the repo is public ("push하지 않으면 볼 수 없어요").
6. AI proposed Tuesday 이음매 (fd copy, rox) for the teammates; he accepted; who got fd vs rox was random ("둘 중에 무작위로", 23:26).
7. Team was already told "fork/wait/exec by Monday" (his line, 23:19).

Page timeline: local `notes/team-plan.html` 10-17 23:29 → site `555273d` 10-18 01:53 (person × day, per-test checks)
→ `690d113` 10-18 02:08 Firebase shared checks, 10 s refresh → `8c6c809` 10-19 10:39 data split to `team-plan.json`
(read by `make pcheck`, `1d9be04` 10-19 10:40; `make ptest` `e3b69f6` 10:53; PRs #106/#107).
Local `notes/` is excluded from git (`.git/info/exclude:25`). Test → issue map 13–83 links each test to a GitHub issue (Board 6).

Vocabulary drift for the talk: whiteboard says **공통/분할**; plan page says **각자/분업/이음매**. 공통 (whiteboard) = 각자 (page): everyone implements the same thing.

## 3. Coding convention

| Claim | Verdict | Evidence |
|---|---|---|
| Triggered by "last week 7조's flow" | **Unknown** | No source mentions 7조 except his 10-22 account. His own week07 team was **9팀** (`cc8s/pintos-team9`; `study/week07/team9.html` title "PintOS 1주차 [Thread] · 9팀"). Ask him: another team (7조), or "7주차 팀"? |
| Shared items: weekly goal, plan vs actual, method check, CONVENTION.md | **Confirmed (board)** | See §1. Plan vs actual became the plan page + `make pcheck`. No separate "weekly goal line" artifact found. |
| From C / Linux / open-source rules | **Confirmed** | 10-15 19:43 prompt: "데니스 리치 혹은 Linux/opensource의 C언어 코딩 컨벤션 … 검사 도구"; 20:04: "오픈소스 C 프로그램의 규약에 맞춘다는 느낌 … 나중에 우리가 C로 소스를 짤 지도 모르니까". `CONVENTION.md:3`: Linux kernel style, two changes (4 spaces, braces always). |
| Formatter (indent/wrap) + linter (snake_case, UPPER macros) bundled | **Confirmed** | `.clang-format` (BasedOnStyle LLVM, BreakBeforeBraces Linux, IndentWidth 4, ColumnLimit 80, InsertBraces). `.clang-tidy`: only `readability-identifier-naming` + `readability-braces-around-statements` (bugprone off: kernel false positives). `Makefile`: `make format`, `make lint` → `scripts/convention.py`. Pinned clangd/clang-format 21.1.8, clang-tidy 21.1.6 in `.devcontainer`. |
| Repo files format/config/.devcontainer/CONVENTION.md | **Confirmed** | PR #104 files: `.clang-format`, `.clang-tidy`, `.editorconfig`, `.git-blame-ignore-revs`, `CONVENTION.md`, `Makefile`, `pintos/compile_flags.txt`, `.devcontainer/{Dockerfile,devcontainer.json}` + whole-tree format. |

**CONVENTION.md structure** (`CONVENTION.md`, 11 KB): three tiers by who enforces —
A 모양 (auto-fixed on save, `.clang-format`: A1 4 spaces … A6 80 cols) ·
B 이름 (yellow underline, `.clang-tidy`: B1 snake_case, B2 UPPER constants/enums) ·
C 이름 짓는 규칙 (review only, `:176`: C1 `모듈_동사[_목적어]`, C2 fixed verb meanings init/create/destroy/open/close/get/set/find…, C3 `_cnt`/`size`, C4 same name for same concept table `fd`/`file`/`status`/`buffer`/`length`/`uaddr`, C5 no struct typedef, C6 named constants).
Every rule has 오답/정답 code. `CONVENTION.md:329`: C rules are for review or "AI에게 이 문서를 기준으로 diff를 검토".

**How it was made (10-15, one evening, with Claude, grilling 2 rounds + subagent audit):**
- 19:43 first prompt; his wish list: verb position in names, `if` spacing, unified variable names, camel/snake, indent, braces.
- 20:11 decisions: braces always; 4 not 8 (tab 8 "너무 과한 듯"); whole-tree format now while no team code exists; teammates are on Windows–Docker–VS Code, so one "Rebuild Container" step.
- 20:35 "linux 방식으로", full format mandatory, separate commit.
- Commits: `6c6c899` 20:47 convention · `a039ceb` 20:47 whole-tree format (in `.git-blame-ignore-revs`) · `ec608a9`/`2ecb5a2`/`88206c7`/`47d3843` 20:48.
- **PR #104** created 20:49, merged 20:55 by benjohnbill (merge commit required so the blame-ignore hash survives). Evidence in body: 18/95 before = after; kernel disassembly identical after masking constants.
- After merge, on his requests: 오답/정답 examples + human naming rules (21:12, `b5985cb` 21:14), Lucide icons instead of emoji (`5e0eed5` 21:17), readable lint/format output with `^` markers (`cb4a8fb` 21:23), "lint fails" note (`3016cb2` 21:41).
- claude-mem #37947 #37949 #37976 #37958 (lint baseline after format: 109 naming warnings from upstream idioms → ignore regexes for `_x`/`x_`).
- 10-16 17:23: test files looked "위화감" unformatted; decided `tests/`, `utils/` stay upstream, he formats locally only (origin of the skip-worktree memory).

## 4. How the convention was used later (notable, not in his account)

- **Merge criteria.** 10-17 21:28 his criteria for building `develop`: "1. CONVENTION.md에 대한 충실도 2. stable 3. 시간/공간 복잡도 4. 가독성". These became the default of the `jungle-team-merge` skill (`skills/jungle-team-merge/SKILL.md:72`, created `eb306dc` 10-18 02:01). Monday meeting card measured "lint · 프로세스 테스트 14개 · threads 회귀 18개".
- **PR #105** (10-17): `make lint` failed on 5 layout lines in JSY's code — teammates' editors were not all formatting on save.
- **PR #110** (10-20): renames by rule id — `struct file_descriptor`→`struct fd_entry` (C4), `process_get_file`→`process_find_file` (C2); lint names 0.
- **Week07 precedent (team 9, his own team).** `circular-list.html` team slide: "① 각자 구현 → ② 리뷰 + AI 순위 (복잡도·가독성·edge case) → 1등 코드 merge → ③ 공통 도달선". Week08 kept this compare-and-pick flow and added the convention + test gate. Week07 had a late naming cleanup (`6ffe37b` 10-14 "함수 이름 정리", `mlfqs_recalc_preq_and_reorder`→`mlfqs_update_all`) — no convention file existed then.

## Open questions for him
1. What is "7조"? (His team last week was 9팀.)
2. What does "3시간 이상" on the board mean?
3. Is the whiteboard photo really day 1 (10-15)? It has no date metadata.
