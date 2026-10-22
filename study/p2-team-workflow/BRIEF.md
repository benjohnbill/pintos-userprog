---
kind: jungle-deck
phase: build
slug: p2-team-workflow
week: 8
deck: p2-team-workflow.html   # relative to the save folder
source_repo: /home/benjohnbill/dev/krafton-jungle/week08
created: 2026-10-22
updated: 2026-10-22
---

# BRIEF — p2-team-workflow

## Next action

Student reviews the first draft at the revise-server address. On his confirmation, set phase: revise.

## Grill

### Context notes

Sources read: the team git log (all authors), PRs #104–#118, `notes/` (p2-debug.md, HANDOFF-2026-10-22.md, rewrite.md), memory, and the week07 BRIEF. Not read: the study site (week08 has only team-plan.html and userprog-flow-map.html; no talk yet).

- Team: benjohnbill 66 commits, Hannuri Song (`nuri`) 15, JSZeroDev (`JSY-*`) 5. Period 2026-10-15 to 10-21. Repo `benjohnbill/pintos-userprog` (public).
- Flow: personal branch → PR into `develop` → `main`. benjohnbill integrates. Integration PRs pick one implementation per feature: #105 (args JSY, write/exit and wait loop nuri), #108 (fork/wait nuri `child_status`, exec JSY), #110 (nuri fd base). Final merge #118.
- Tooling: `pintosdev` container (host gcc 15 cannot build), `make dcheck`/`dfast`/`dg_<test>`, `-icount shift=5`, `make pcheck` syncs results to the plan page; clang-format/tidy (#104); PR template with test counts; add-only tests under `tests/userprog/` (`read-code-seg`, 59ebc9d).
- Progress: 46/95 at #108 → 50 at #110 → 23 FAIL after #113 → #118 94/96 at TIMEOUT=60 (multi-oom, syn-read time out; both pass at TIMEOUT=600).
- Troubleshooting candidates: (1) bad-* x6 — user fault took the `kill()` dump path; the 64-bit dump (`rip=`) slips past the `eip=` filter in tests.pm → fix in `page_fault()` (2a10f55), board 21/21; nuri fixed it a different way (15308de). (2) exec tests print the child name — JSY and nuri fixed the same bug separately; #110 kept one whole function. (3) exit message broke priority-donate-one — print only for processes with a page table (e37bea7). (4) args tests hang on the `process_wait` placeholder — judged with TIMEOUT=10. (5) read into the code segment — W-bit check + added test `read-code-seg`. (6) fd cleanup ran twice after a clean merge — #114. (7) three wait designs in #108. (8) write with an unmapped buffer must exit(-1) (d5f1779). (9) final timeouts.
- AI rule: the course forbids relying on AI code generation; concept-tutor mode on team branches.
- Student's material (2026-10-22, not in slide order):
  - Day 1 whiteboard: unit of team activity = meetings and PRs, set by the unit of work → split work into 공통 and 분할. 분할→공통 when logic/variables are shared (write all, then review); 공통→분할 when time runs short (AI proposes a split by test cases). fork/wait/exec = 공통; prerequisites args-passing + write/exit/wait loop → day-1 to-do (gitbook, how Shell/userprog runs, syscalls). Convention block: weekly goal line, plan vs actual per day, method check, CONVENTION.md (formatter + linter from C/Linux/open-source rules; repo format/config/.devcontainer/CONVENTION.md).
  - Day 2: team commands `make ptest`/`dptest` (icount shift, parallel QEMU, TIMEOUT 5 s) that sync results live to the plan page/calendar through a Firebase server (`make pcheck WHO=<name>`). Morning quiz from userprog-flow-map.html (concept check 12, 3 solved, he explained at the whiteboard) before the coach's 1 pm lecture. Days 1–3: whiteboard explanations (no photos).
  - Git: GitHub Flow style. Personal branches → `develop` after daily meeting/merge; `main` only at start and end. In the split phase, merge by feature and compare tests against the AI-predicted set, with separate merge commits.
  - Time pressure: fork/wait/exec due by Monday 21:00 core time; whoever is not done gets force-merged/pulled. Result: fork/wait nuri, exec JSZeroDev; his code kept only some exception handling and exit/write. Merge priority by three measures (Monday meeting card: lint · 14 process tests · 18 threads regression), done with AI; CONVENTION.md made names line up — an AI use that worked.
  - Tue–Wed: split work, real-time review and feature-sized merge/pull against expected tests; most active communication. All tests passed Wednesday evening.
  - Problem he felt (early Wednesday): split work → each knows the others' code only superficially; review stays at name level. He planned a fill-in-the-blank rewrite branch as a refresh before VM; it faded once the tests passed and talk prep started.
  - All team decks are made with his jungle-deck skill: two members write HTML, he merges.
  - Images: plan page (to-do with board counts; "3. 나눈 기준" 각자/분업/이음매/주말 선구현) `/mnt/c/Users/benjohnbill/AppData/Local/cc-paste/20261022-022447-509.png`, `…-022458-824.png`; whiteboard `/mnt/c/Users/benjohnbill/Downloads/IMG_8052.jpg`; concept quiz `…-025748-769.png`, `…-025758-756.png`. Asked for: a GitHub Flow diagram with the real merge log.
- Fact sheets (2026-10-22, each claim → confirmed/corrected/unknown with evidence): `research/A-plan-method-convention.md`, `research/B-tooling-board.md`, `research/C-git-timeline.md`, `research/D-learning-rewrite.md`. Corrections to put to the student: "3. 나눈 기준" is from 10-17 (grilling), not day 1; tooling built 10-17..10-19, not day 2; Firebase RTDB polled every 10 s, no own server; `dptest` is personal (GNUmakefile, excluded); #108 merged without evaluation ("머지만 해주라"), the criteria-judged merge was #105 (4 criteria); "21:00 force merge" only in his own account; #115 merged to main by mistake, Copilot resolved #118 conflict without a test re-run; rewrite started Tue 10-20 22:17, solo, motive his own fragmentation; week07 team was 9팀 (no "7조" trace); team9.html stacks decks in iframes.
- Student's answers to the corrections (2026-10-22):
  - Convention idea came from another team's talk last week that said a convention seemed needed; he built it. The agent's dates are right.
  - Split: roughly set Thursday; redone Saturday night when the process syscalls began, because by then everyone knew a first PASS needs wait (loop)/write/exit. "원칙을 먼저 세우고, 개념이 잡힌 뒤에 실제로 나눴다" is roughly right.
  - `dptest` is his only. Tools were made mid-way, when args/basic syscalls were nearly done and the tests had to run: "중간에 필요를 느껴서 만들었다".
  - Teammates used `pcheck`/`ptest`; `ptest` was everyone's default command unless they looked at one test.
  - Merges: #105 judged by AI on 4 criteria; #108 the deadline merge. Do not explain the team-merge skill. The 21:00 rule was oral; do not write "구두로 정함" on a slide.
  - Rewrite: Tuesday evening, local branch, from his own regret ("내가 피상적이었다"). Wednesday morning he proposed it to the team; they said they would join if time remained; after the finish there was no time.
  - Cut: the main mistakes (#115, #118 conflict). Cut: "3시간 이상" (his time cap for building the convention tooling).
  - Quiz: he prepared 12 questions; too hard, even he solved only 3. Those 3 held the core of Friday's args-passing; teammates said his Friday-morning explanation helped.
- Last week (week07 circular-list): a team talk, 6 min total; benjohnbill took 30 s team result + ~110 s personal, 8 slides.

### Metadata

D7 (2). Settled in the same round as the context check-in.

- Format: team talk, as in week07. The student covers the team's way of working and its result, with some of his own story. Speaking time 2 min 20 s.
- Title (student's): "1주일간의 8팀" — subtitle "Pintos Userprog".
- What to present: the team's week in time order, split by date (about two slides per date). Close with his own "내가 피상적이었다" as regret and resolve: work harder next week.
- Why: to show how the team collaborated, and how he built the tools and ran the process for the team's collaboration.
- Student's view (steers the takeaway): he considers the team a model case; the team's practice — exactly, the tools and system he built — is the method or tool the audience can take away.
- The one line the audience should take away: "누가 짠 코드든 합칠 수 있게: 같은 컨벤션, 같은 테스트 명령, 같은 보드." He has much to say on it. His regret also grew from it: "팀적으로 결과가 잘 나오고 있는 것 같은데, 내 이해는 따라가지 못하는 것 같다".
- Slide count: 7 + title = 8, in four date blocks: 목·금 2 (principles and convention, quiz), 토 2 (re-split, tools), 월 1 (deadline merge), 화·수 2 (split work and all pass, the blank-fill regret and resolve). Real time per slide follows weight and his emphasis, not an even split.

### Story frame

D7 (3). Two questions. A frame fixes no slide count.

- Structure: time order by date block (목·금 → 토 → 월 → 화·수).
- Opening: link to last week. After all of last week's talks, the coach said publicly that teams should settle a convention in advance; one team's talk also said a convention seemed needed. "저희는 첫날 그걸 만들었습니다" → the 목 block.

### Title slide and closing line

D7 (4). The title slide states the point, lead-first. Then the line that closes the talk.

- Title slide: "1주일간의 8팀" / "Pintos Userprog" + lead line "누가 짠 코드든 합칠 수 있게: 컨벤션 · `ptest` · 보드"
- Closing line: "누가 짠 코드든 합칠 수 있었습니다. 하지만 팀적인 통일과 개인의 이해도는 완전히 다른 영역이었습니다. 스스로가 부족했습니다. 다음주에는 원리와 실패 원인을 모두 명료하게 설명할 수 있도록 공부하려고 합니다."

### Slide theses

D7 (5). One sentence per slide.

| # | Section id | Role | Thesis |
|---|---|---|---|
| 01 | s1 | Title | 1주일간의 8팀 — Pintos Userprog / 누가 짠 코드든 합칠 수 있게: 컨벤션 · `ptest` · 보드 |
| 02 | s2 | 목 | 지난주 코치님 말씀대로, 첫날 나누는 원칙과 컨벤션부터 깔았다. |
| 03 | s3 | 금 | 개념은 쪽지 시험으로 점검하고, 모르는 건 칠판에서 같이 풀었다. (Do not show "he solved only 3 of 12": student said it is a different point from the closing's 부족함.) |
| 04 | s4 | 토 | 첫 PASS에 필요한 것이 보이자 다시 나눴고, 합칠 때는 AI가 컨벤션을 첫 기준으로 골랐다. |
| 05 | s5 | 토~월 | 테스트를 돌려야 해서 도구를 만들었다: 명령 한 번에 26초, 보드는 10초마다. |
| 06 | s6 | 월 | 월요일 마감 merge에서 내 fork·wait는 남지 않았지만, 컨벤션 덕에 바로 읽혔다. |
| 07 | s7 | 화·수 | 기능 단위로 나눠 합쳤고, 수요일 저녁 96개를 통과했다. |
| 08 | s8 | 마무리 | 팀은 합쳐졌지만 내 이해는 따라가지 못했다 → 빈칸 세션 → closing line. |

### s5 video plan (student agreed 2026-10-22, later)

The board is all checked now, so a plain rerun shows no change. Plan: run `make dptest` at develop `aaea825` (#108, 46/95) to set the board back, then record a run at the final develop with `TIMEOUT=600` (two long tests fail at 5) while the page fills; cut to about 5 s with openscreen. Build leaves a video slot on s5; the agent saves the run log to `evidence/`. Tell the team before, since the shared board drops for a while.

### Team page (for publish)

Three decks joined in this order, like `study/week07/team9.html` (iframe stack): 1) this deck (조진근), 2) 누리님 `/mnt/c/Users/benjohnbill/Downloads/pintos-userprogs (1).html`, 3) 승영님 `/mnt/c/Users/benjohnbill/Downloads/8team_pintos-wait_정승영 (1).html`. He plans to post the joined page on benjohnbill.github.io in the team9 form.

### Evidence list

D7 (6). Real output only. Each item names its source run and its slide.

| Slide | Evidence | File in `evidence/` | How it was made |
|---|---|---|---|
| s2 | Day-1 whiteboard (cropped: no "3시간 이상") | `whiteboard-day1.jpg` | student's photo IMG_8052.jpg |
| s2 | #104: 18/95 before and after the format | `pr-104.json` | `gh pr view 104 --json …` |
| s3 | Concept quiz page | `quiz-1.png`, `quiz-2.png` | student's screenshots |
| s4 | Plan page "나눈 기준" (cropped: 각자 and 분업 only, no "이음매") | `plan-page-criteria.png` | student's screenshot |
| s4 | #105 pick table, args/exit FAIL 6/6 → PASS 6/6; 4 AI criteria | `pr-105.json`; `../research/C-git-timeline.md` | `gh pr view 105`; transcript per research C |
| s5 | Plan page board (stands in for the video) | `plan-page-board.png` | student's screenshot |
| s5 | #107: 26 s, 95 results | `pr-107.json` | `gh pr view 107` |
| s6 | #108 source table | `pr-108.json` | `gh pr view 108` |
| s6 | #110 renames by rule (C4, C2) | `pr-110.json` | `gh pr view 110` |
| s7 | develop merges and dates; main merges | `develop-merges.txt`, `main-first-parent.txt` | `git log --first-parent origin/develop --merges` (KST) |
| s7 | 18/95 (#104), 46/95 (#108, notes/p2-debug.md), 50/95 (#110), 73/96 (#113), 94/96 at TIMEOUT 60 → 96 at 600 (#118) | `pr-*.json` | PR bodies |
| s8 | rewrite/fwe log | `rewrite-fwe-log.txt` | `git log rewrite/fwe --not origin/develop` (KST) |

### Terms to define on screen

D7 (7). Terms the audience may not know. Each gets a short on-screen definition.

| Term | On-screen definition | Slide |
|---|---|---|
| `ptest` / `pcheck` | 팀 테스트 명령 / 결과를 분업표에 반영 | s5 |
| icount | QEMU 가상 시계 모드: 테스트 17~25배 빠름 | s5 |
| develop | 매일 합치는 공용 브랜치 | s7 |

Student (2026-10-22): never use the word "이음매" (he says it aloud; it reads AI-like). Board 6 → just "GitHub Project", and it is not a talking point: the live HTML page did that job.

### Script

D7 (8), optional. Ask "발표 스크립트를 먼저 만들까요?". Yes: a full per-slide script here. No: write `없음 — build가 가벼운 노트를 씀`. After build, replace the script with `build에서 노트로 옮김 — 이후 수정은 페이지에서` (D9).

build에서 노트로 옮김 — 이후 수정은 페이지에서

## Design

Filled in `design` (D17). `build` reads the style only from this section (D9).

- Source: ~/dev/krafton-jungle/docs/slides/DESIGN.md (Plex Swiss Grid; same as week07, and the teammates built their HTML from it)
- Tokens: `--paper: #ffffff` (neutral) · `--ink: #0a0a0a` (primary, the file's ink) · `--acc: #ff3300` · `--mut: #6b6b6b` (muted) · `--panel: #f7f7f5` (panel) · `--grid: #ebebeb` (grid). The student chose the file's own panel/grid/ink values over the mapping fallbacks. Fonts: `'IBM Plex Sans KR', 'Apple SD Gothic Neo', 'Malgun Gothic', sans-serif`; code `'IBM Plex Mono', monospace`.
- Accent: #ff3300 (same as the team's decks, for unity)
- Accent key: accent

## Build

What `build` produced. Filled by the agent.

- Page file: `p2-team-workflow.html` (8 sections s1–s8; render checked at 1280×720 and 1920×1080, no overflow)
- Open issues for `revise`:
  - s5: the board screenshot stands in for the video; swap in the clip when recorded (see "s5 video plan").
  - s3, s4: the screenshots carry small text; at the big monitor only their shape reads. Consider HTML boxes if he wants them read.
  - IBM Plex Sans KR glyphs overflow the template line box; headings got `line-height` and `padding-bottom: .1em` (deck CSS § 13).

## Revise

One line per round. The student saves in the browser; the agent reads the change and adjusts layout and evidence.

| Round | Date | What changed |
|---|---|---|
| 1 | TBD | TBD |

## Publish

What `publish` did (D12–D14).

- Commit in this repo (the commit, or "not a git repo"): TBD
- Separate deploy (student's answer): TBD
- Pages copy path: TBD
- Pages repo commit: TBD
- Push approved by student (one answer for commit and push, this pass): TBD
- Live URL (HTTP 200, `cmp` match): TBD
- Index card found: TBD
- DESIGN.md feedback: TBD
