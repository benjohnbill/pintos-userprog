# D — Concept learning, the quiz, the fill-in-the-blank refresh, the course AI rule, team decks

Researched 2026-10-22. Sources are read-only. Calendar: Day 1 = Thu 2026-10-15, Day 2 = Fri 10-16, ... Mon 10-19, Tue 10-20, Wed 10-21.
"obs #N" = a claude-mem observation. "P#N" = a claude-mem prompt record. Transcript times are UTC (Z); KST = Z + 9h.

## 1. Claims checked

| # | His claim | Verdict | Evidence |
|---|---|---|---|
| 1 | Day 2 morning: a quiz from `userprog-flow-map.html`, "개념 체크 12문항" | **Confirmed** | Page subtitle: "금요일 오전 개념 체크 12문항 포함" (`docs/userprog-flow-map.html:68`); heading `:231`. obs #38021 (handoff, 10-15 21:15): "Team session deadline Fri 2026-10-16 10:00". |
| 2 | Format: 30 min solo, then hint, then answer | **Confirmed** | `docs/userprog-flow-map.html:232`: "30분 각자 풀이 → 문항별로 첫 겹(단서) → 둘째 겹(답) 순서로 펼치기". Each question has a 단서 fold and a 답 fold, plus a per-question answer box saved in localStorage that is echoed on top of the 답 fold (`:455-480`, added in site commit 569af27, 10-15 23:20). |
| 3 | Each member solved 3 questions | **Unknown (partly consistent)** | No record of the split. 12 / 3 members = 4, so "3 each" leaves 3 questions unassigned, or the split was different. His screenshot of the page (`cc-paste/20261022-025748-769.png`, `-025758-756.png`) shows his answer boxes filled for Q1–Q3 and empty for Q4. That fits "he took 3". |
| 4 | He solved them the night before and explained at the whiteboard | **Confirmed (night before); whiteboard unknown** | P#12452, 10-15 22:15 KST: "1번부터 풀어볼게". obs #38056 (10-15 23:12): a live Q&A while he solved them found 5 gaps in Q1 (prerequisites, syscall number vs syscallN's N, counting args from a1, RCX vs RAX). obs #38092 + commit 1854f0b (10-15 23:54): Q1 and Q2 were refocused from his solving, and each clue got a bold "which test needs this" line. The whiteboard has no digital trace except the photo named in BRIEF.md (`/mnt/c/Users/benjohnbill/Downloads/IMG_8052.jpg`, not inspected for content). |
| 5 | Goal: measure understanding/metacognition after reading the gitbook; prepare for the coach's ~1 pm userprog lecture | **Confirmed (timing); goal wording is his** | obs #38021: "30min solving → answer check → Q&A; coach lecture 13:00–15:00; team splits work into 공통 (everyone codes) and 분업 (delegate + tests) after 15:00". The page was "a reference (not binding) for the team's split decision at 15:00". The week07 course schedule had the same slot ("Pintos 기초 특강 … 금요일 오후 1시", transcript `-home-benjohnbill-dev-krafton-jungle/36cf70bc-….jsonl`, 10-08). No source states "metacognition" as the goal. |
| 6 | Days 1–3: whiteboard explanations whenever someone did not understand | **Unknown** | No digital record. Only the whiteboard photo above exists. |
| 7 | Early Wednesday he felt split work made everyone know others' code only superficially; review stayed at name level | **Corrected (date and motive)** | The plan started **Tuesday 10-20 ~22:17 KST**, not early Wednesday. His words (transcript `-home-benjohnbill-dev-krafton-jungle-week08/b5942ee4-….jsonl`, 10-20 13:17Z): "fork/wait/exec의 로직을 대략적으로는 알지만, 다시 짜라고 하면 짤 자신이 없어. 파편적인 측면도 커. … fork는 내가 짠게 아니기도 하고, exec도 중간에 끊겨서 팀원들한테 머지당했거든. 그에 따라, 한 번 우리 팀 레포하고는 별개로, 다시금 짜보려고 해." The stated motive is **his own** fragmentary knowledge from the split, not "everyone's". Purpose (13:21Z): "'내가 어디에서 빈공간이 존재하는 지'를 파악하는 훈련". Counter-evidence to "review stayed at name level": the same afternoon (10-20 06:53Z) he questioned *why* the team's fork used three semaphores, and built `exp/shared-sema` (214966a, 10-20 19:08) to test merging them into one. |
| 8 | He planned a new branch with blanks punched out to refill | **Corrected (it was a solo drill; blanks came mid-way)** | Branch `rewrite/fwe`, local only, "NEVER push" (`notes/handoff-week08-rewrite-stage2.md:3`). It started as **"delete and rewrite from the upstream skeleton"**, with him writing all code; the agent only reviewed. The fill-in-the-blank format was adopted mid-stage 2 when time ran short (6705f988 transcript, 10-20 16:57Z = 10-21 01:57 KST: "코드도 곁들이면서 빈칸 뚫려있는 느낌으로 가는게 맞곘는데..?"; 17:43Z: "3단계도 이번처럼 빈칸채우기로 가야할 듯. 그리고, 4단계는 … 생략"). Not a team branch. |
| 9 | A wrap-up and concept review before next week's VM | **Partly confirmed** | The "before VM" framing applies to the *later* transcription-mode trial: `notes/HANDOFF-2026-10-22.md` item 9 says promotion of the method is decided "after one VM week"; `trial.md` plans fork, then wait and file functions, blanked again. The original rewrite (10-20) framed itself as "오직 하루" / 6 hours, not as VM prep. |
| 10 | It faded after tests passed Wednesday evening and talk prep began | **Confirmed, with detail** | Last rewrite commit: 54f21db "blank set", Wed 10-21 10:20 (fork skeleton with 22 `____` blanks; never filled). Team then worked file syscalls all day; develop e223db2 "Final Result", Wed 10-21 19:53. The trial branch does not exist yet; `.claude/docs/trial.md` status: "Trial, not started" (branch name, base, push: all open). |
| 11 | All team talks are made with his jungle-deck skill | **Confirmed for week07** | `lock-holder-intr.html` (Jun) references `jungle-deck/DESIGN.md` and shares the deck runtime; obs #39151: "Jun and Yena decks share runtime (E edit, Ctrl+S export, N notes)". obs #37079: DESIGN.md is the sharing artifact for team style. |
| 12 | Two members write HTML each, he merges | **Corrected (not a file merge)** | Each member's deck stays a separate HTML file. `team9.html` (site commit e93dfe1, 10-15 11:37) stacks the three decks in iframes and moves to the next deck when a key press no longer changes the current deck (`benjohnbill.github.io/study/week07/team9.html:7-12`, obs #37790). It was "6분 팀 발표", three parts: 1. circular-list (him), 2. lock-holder-intr (Jun = 조성준), 3. only-what-changed (Yena = 신예나) (obs #37791, session S10541). Teammates' latest exports were pulled in by `pull_edits.py` / copy (obs #37795, 589fba4, 10-15 13:03); a deck-transition bug was fixed at 13:06 (ba957fe, obs #37798). So "he merges" = he composes the three decks into one wrapper and publishes, not one merged HTML. |

## 2. userprog-flow-map.html — how it was made

- Built by the agent in his tutoring session on Day 1, Thu 10-15, after the morning week07 talk. Draft obs #37994 (20:55). Week08 repo commit 3ac903e (21:47), site commit 0116627 (20:59). Answer boxes: site 569af27 (23:20). Q1/Q2 refocus + test rationale per clue: 1854f0b (23:54), site 4d613b9.
- Content: §1 execution-flow map of one command line ("명령줄 한 줄의 일생") with nodes A–K; §2 64 userprog tests in groups; §3 the 12 questions. Sources: 6 Project 2 gitbook pages (`:68`).
- Stated basis of the "actual output" lines: the repo kernel run as-is, or with only `process_wait` made to wait (`:233`, obs #38021).

The 12 questions (`docs/userprog-flow-map.html:235-455`):

1. Registers of a normal call vs a syscall (6 args): what differs, which value only a syscall has, where `f` shows it.
2. Can the user walk `argv` to "onearg" on the kernel-built stack (USER_STACK 0x47480000)?
3. Where the command line must be split, and where each result goes (`open failed` + `initd` PANIC output).
4. exec-once: the last two output lines, and why `(exec-once) end` is missing.
5. Same "Page fault at 0", different cause (bad-read on a kernel without arg passing).
6. bad-read vs read-bad-ptr: who dereferences the bad address (user or kernel), and where to block it.
7. First `open` return value, and the second open of the same file (fd 0/1 → FAIL).
8. After fork + exec (exec-read): from which byte the parent reads on the shared fd.
9. Child exited with 81 and was reaped before `wait`: where does 81 live?
10. When may fork return: where the parent waits and where the child signals.
11. rox-simple: `file_deny_write` alone in `load` — why the write still succeeds.
12. No output at all, not even `open failed` (`make dr_args-single`): which function?

## 3. Rewrite plan (`rewrite/fwe`)

Plan in `notes/rewrite.md:1-13` (obs #39125, 10-20 22:40), 6 hours total:

| Stage | Time | Target tests | What happened |
|---|---|---|---|
| 1. record + minimal wait/exit | 1h15 | args-* | Done by him: 5e6a6a8 (Wed 10-21 00:51), args 5/5 pass |
| 2. exec | 0h45 | exec-* 5 | Done; switched to fill-in-the-blank mid-stage. f0c3f04 "SYS EXEC commit." (02:51); 4/5 exec + args-none pass (exec-boundary needs fork) |
| 3. fork | 1h45 | fork-* 4 | Agent gave the blank skeleton: 54f21db "blank set" (10:20). Not filled |
| 4. wait completion | 1h15 | wait-* 4, multi-recurse | Skipped by his decision ("이전에 wait을 직접 짜봤어서") |
| buffer | 1h | — | Became ~0 ("여유시간 1분", 10-20 14:56Z) |

Branch commits (all his, local): c1b135f 10-20 22:30 reset fork/wait/exec to the upstream skeleton → 5e6a6a8 10-21 00:51 → f0c3f04 10-21 02:51 → 54f21db 10-21 10:20. Base: his branch after merge 864693e (10-20 22:11).

Drill rules (settled by grilling, 10-20 13:17–13:26Z): separate branch from his personal branch; no peeking at team code or his own old branches; agent never writes `pintos/` code (later waived only for the blank skeleton, `rewrite/fwe` only); he timed each stage and called "10분전" for options.

## 4. Trial / transcription mode status

- Designed in session 9a9b8353 by grilling Q1–Q44 (10-21 night → 10-22 01:45), committed in krafton-jungle 178d109 (10-22 01:07): `docs/transcription-mode.md` (145 lines), `docs/transcription-pintos.md`; spine became a router.
- Method: agent generates code; he types every line after its questions are answered and judges it against a spec sheet written first (`notes/HANDOFF-2026-10-22.md:9-12`). Basis cited: Shen & Tamkin, *How AI Impacts Skill Formation*.
- Status: **not started** (`.claude/docs/trial.md`, "Decided at session start" all open). Plan: new local branch, fork first, revise the method doc, then wait and file functions. Candidate base: `rewrite/fwe` 54f21db.
- He also considered sharing the breaks/tests branch with teammates (209f605c transcript, 10-21 14:14Z: "내가 팀원들한테 branch에 공유할 수도 있고"). Trial is told to the team in person only (`trial.md`).

## 5. The course AI rule

- P2 guide text, pasted by him 10-15 12:47Z (transcript `-home-benjohnbill-dev-krafton-jungle/9aa3c04a-….jsonl`): learning goal 3 — "AI Agent나 코드 자동 생성에 의존하지 않고 직접 코드를 구현하는 경험을 통해 프로그래밍 문법, 알고리즘, 디버깅, 그리고 코드 검증 역량을 기릅니다." His comment: "이 정도 온도로 적혀있네. 물론, tutor 로서의 역할은 내가 요구하겠지만."
- The same sentence was in the week07 (Threads) goals (pasted 10-08). Week07 also had a separate guide "Pintos 개념 학습을 위한 AI 활용 가이드" that bans "Alarm Clock 구현해줘"-style requests (`week07/CLAUDE.md:19-21`). The agent recorded P2 as "milder than week 7's explicit ban" (obs #38037).
- How it was handled: concept-tutor mode on every team branch — the agent answers concept questions with file:line, never writes `pintos/` code (`week08/CLAUDE.md` § Mode). Exceptions are explicit and recorded: `exp/shared-sema` (comparison only, never pushed) and the `rewrite/fwe` blank skeletons (progress memo, "Waivers"). In jungle-team-merge the agent only assembles lines teammates wrote; logic fixes need his "알아서 해줘" at the gate (`skills/jungle-team-merge/SKILL.md:16-24`, commits eb306dc/d24e633/5edbbbb, 10-18).
- His reading of "직접" (10-21/22 grilling): he types every line himself after its questions are answered; the failure to prevent is copy-paste (`week08/CLAUDE.md` § Mode).
- Week07 precedent: two agent-written exceptions were recorded openly (cond_signal ordering, uncommitted; MLFQS fix a02036e, PR #42 with Co-Authored-By) (`week07/CLAUDE.md:55-61`).

## 6. week07 "7조" lessons

**Unknown.** No match for "7조" or "7팀" in week07, the study site, claude-mem, or any krafton-jungle transcript except this research session. Note: his own team in week07 was **9팀** (team9.html title "PintOS 1주차 [Thread] · 9팀").

## 7. Notable facts he did not mention

1. The quiz page was built the evening before, iterated from his own solving that night (Q1 gaps → refocus), and published on his study site — the quiz is itself a product of a tutor loop.
2. Each question carries tags: kind (정의/예측/위치), flow nodes (A–K) and target tests, and every clue opens with one bold line naming the test that needs it.
3. The quiz fed the 15:00 split decision into 공통 / 분업 (obs #38021); `notes/team-plan.html` (published 10-19, obs #38755) maps all tests to board issues #13–#83.
4. The rewrite was a reaction to integration: by Monday 21:00, fork/wait came from nuri and exec from JSZeroDev (BRIEF.md:34; #108 e11e98d/c322930), so his own fork/exec code did not survive.
5. Day 1 (10-15) was also the week07 talk day: the team9 deck was finished 11:37–13:06 the same morning.
6. jungle-deck was tested on Codex CLI on 10-21 01:00 (obs #39154–#39159) — likely for teammates on Codex; a Slack intro was drafted (obs #39151).
