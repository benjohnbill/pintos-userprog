---
kind: research
topic: git/GitHub collaboration timeline, Monday forced-merge deadline, merge criteria
period: 2026-10-15 (Thu) .. 2026-10-21 (Wed), times KST
sources: local git refs (fetched state as of 2026-10-22), gh pr list/view #104-#118, notes/team-plan.json, Claude transcripts (~/.claude/projects/-home-benjohnbill-dev-krafton-jungle-week08/*.jsonl, cited by first 8 chars of the session id), claude-mem observations (#id)
written: 2026-10-22
---

# C. Git timeline, deadline, merge criteria

Repo `benjohnbill/pintos-userprog`. Members: benjohnbill (integrator, merged every PR), Hannuri Song (`nuri`), JSZeroDev (`JSY-*`).
Non-merge commits now on `main` since 10-15: benjohnbill 34 (8 of them baseline/convention/docs on 10-15), Hannuri Song 13, JSZeroDev 5.
Every merged PR was merged by benjohnbill (`gh pr list --json mergedBy`).

Caution about claude-mem: observations #39566 and #39568 ("force-merge on Monday 9pm", "async review replaced meetings") were created 2026-10-22 03:16 KST from the student's own account in the deck session. They repeat his claim; they are not independent evidence.

## 1. Day-by-day timeline

### Thu 10-15 — setup, all on `main`

| Time | Hash / PR | Author | Event |
|---|---|---|---|
| 19:27 | `7f88707` | benjohnbill | Import Project 1 baseline (cc8s/pintos-team9@ebe0035), root commit, direct to main |
| 19:50 | `4a50cae` | benjohnbill | PR template test line, direct to main |
| 20:47–20:48 | `6c6c899`..`47d3843` | benjohnbill | Convention, whole-tree format, blame-ignore, devcontainer |
| 20:49 → 20:55 | PR #104 `chore/c-convention` → main, merge `7465e75` | benjohnbill | clang-format/tidy, `make lint`, CONVENTION.md. Body: 18/95 pass, unchanged by the format |
| 21:14–23:54 | `b5985cb` `5e0eed5` `cb4a8fb` `3016cb2` `3ac903e` `1854f0b` | benjohnbill | 6 more direct pushes to main (CONVENTION examples, lint output, flow-map study log) |

`develop` was branched from `main` at `1854f0b` (first seen locally 10-17 06:30, reflog of `origin/develop`).

### Fri 10-16

| 22:24 | `8164e53` | Hannuri Song | argument passing (nuri) |

### Sat 10-17 — first integration (#105)

| Time | Hash / PR | Author | Event |
|---|---|---|---|
| 11:36 | `3d5896d` | Hannuri Song | arg passing moved into load |
| 15:02 | `089f37d` | JSZeroDev | arg passing (JSY-argument-passing) |
| 16:05, 19:28, 21:21 | `a19a88f` `eddc914` `f36caa5` | benjohnbill | args, write/exit, thread name (benjohnbill branch) |
| 20:57 | `c486a11` | Hannuri Song | write + exit syscalls |
| 21:28 | transcript 7d7fd804 | benjohnbill | Asks Claude to compare the 3 branches by 4 criteria (see §4) |
| 21:58–22:18 | `d77f7c6` `43a47f8` `fe74c7f` `a2ff454` `e37bea7` | benjohnbill (Claude ran git) | `integrate/args-syscall`: merge JSY → merge nuri → merge benjohnbill → format → exit-message fix |
| 22:02 → 22:21 | PR #105 → develop, merge `7ac9c04` | | args from JSY, write/exit + wait loop from nuri, 1 name line from benjohnbill |
| 22:34 | `c0e9a73` "." | benjohnbill | personal branch |
| 22:45–23:26 | transcript 545787f1 | | Grilling session that produced the plan in `team-plan.json` (§4) |

### Sun 10-18

No commits by anyone. (The plan had benjohnbill pre-building fork/wait/exec over the weekend; his first process commit is Mon 17:05.)

### Mon 10-19 — tools, then the fork/wait/exec deadline (#108)

| Time | Hash / PR | Author | Event |
|---|---|---|---|
| 10:42 → 10:55 | PR #106 `tools/plan-check` → develop, `1334cec` | benjohnbill | `make pcheck` (plan page sync) |
| 10:57 → 10:59 | PR #107 → develop, `714c06a` | benjohnbill | `make ptest` |
| 16:58, 16:59 | `b987722` `6cd1751` | Hannuri Song | wait syscall; pulls develop |
| 17:05 | `6fcb968` "Complete wait()" | benjohnbill | his wait (`struct record`, `child_list`) |
| 18:42 | transcript 3f6b74d6 | benjohnbill | "오늘 저녁까지 exec는 확실히 끝내야 하고. 저녁 9시까지 fork도 다 끝내고 싶기는 하지만, 이건 희망사항" ; 19:02 "2시간 안에 exec() 구현을 완료해야" |
| 21:15 | `91d5cdb` | JSZeroDev | wait (`child_info`) + exec, branch `JSY-wait-exec` |
| 21:16 | `b146510` | Hannuri Song | fork |
| 21:55 | `249b2d2` "." | benjohnbill | his last pre-merge commit (exec stage 1, HALT, name change) |
| 22:19–22:25 | `179e6f7` `c322930` `e11e98d` | benjohnbill (Claude ran git) | `integrate/fork-wait-exec`: merge nuri (clean) → merge benjohnbill, all conflicts to nuri's side → merge JSY taking only `SYS_EXEC` |
| 22:28 → 22:32 | PR #108 → develop, merge `aaea825` | | fork+wait nuri, exec JSY. Body: builds; `exec-once` FAIL |
| 22:57 | `95dccd3` "." + PR #109 `integrate/fork-wait-exec` → **main**, closed unmerged 22:57 | benjohnbill | stray PR to main (template body empty) |
| 23:13 | transcript cec9e2f2 | | Board 21 tests on develop after #108: 11 pass / 10 fail; 49/95 fail overall |
| 23:23 | `806c8bb` | JSZeroDev | **direct push to develop** (no PR): keep process name after exec |
| 23:27 | transcript cec9e2f2 | | Re-measure: 15/21 pass; only bad-* ×6 left. Full suite 50/95 (#38918) |

### Tue 10-20 — fd base (#110)

| Time | Hash / PR | Author | Event |
|---|---|---|---|
| 13:28 | `541a78c` | Hannuri Song | `extract_thread_name` refactor |
| 15:43 | `2a10f55` | benjohnbill | user page fault → exit(-1) without dump (personal branch) |
| 17:11 | `f3ea9c8` | Hannuri Song | fd table base |
| 19:08 | `214966a` | benjohnbill | `exp/shared-sema`, local experiment, never pushed |
| 21:39 | transcript 4a6080d9 | benjohnbill | "Nuri의 개인 브랜치에 있는 커밋들을 develop으로 머지하고 pull하고 싶어 … /jungle-team-merge" |
| 21:41, 21:54 | `75b1b2d` `899bb3c` | benjohnbill | `integrate/nuri-fd`: merge nuri; rename fd names to CONVENTION (C4, C2) |
| 21:55 → 21:59 | PR #110 → develop, `daf0f23` | | 50/50 regression kept |
| 22:09 | `15308de` "exception" | benjohnbill | **direct push to develop**: `page_fault()` user → exit(-1) |
| 22:09–22:55 | `e3a27f5` `bddff87` `d869ffa` | Hannuri Song | open/close, pull develop, read |
| 22:30–10-21 10:20 | `c1b135f` `5e6a6a8` `f0c3f04` `54f21db` | benjohnbill | local `rewrite/fwe` (fill-in-the-blank rewrite of fork/wait/exec), never pushed |

### Wed 10-21 — split phase, 6 PRs, final merge

| Time | Hash / PR | Author | Event |
|---|---|---|---|
| 09:39 | `d0abcea` | Hannuri Song | fd duplication on fork |
| 10:29 → 10:32 | PR #111 nuri → develop, `4cb685a` | Hannuri Song | open/close/read + fork fd dup (test result "확인 필요") |
| 11:13 | `3ac2eba` | Hannuri Song | close all fds at exit |
| 11:20, 13:46, 13:59 | `b2736f6` `59ebc9d` `860e86b` | benjohnbill | fd cleanup at exit, read W-bit check + added test `read-code-seg`, filesize |
| 13:43 | `c92cdac` | JSZeroDev | **direct push to develop**: deny write to running executable (rox) |
| 13:55 | `4490919` | Hannuri Song | seek |
| 14:06 → 14:23 | PR #113 benjohnbill → develop, `dffb3e5` | | 26/95 FAIL → 23/96 FAIL |
| 14:00 → 14:24 | PR #112 nuri → develop, `052a71f` | | seek |
| 14:25 → 14:26 | PR #114 benjohnbill → develop, `06adc15` | | drop duplicate fd-cleanup loop (#112 and #113 both added one) |
| 15:12 | `b0624ab` | JSZeroDev | remove |
| 15:14 → 15:23 | PR #115 `JSY-remove` → **main**, `af3c711` | | **mistake: merged straight into main** |
| 15:20 / 15:22 → 15:22 | `13e8a32`, PR #116 nuri → develop, `04e4df2` | Hannuri Song | create; body: all pass except multi-oom, write-bad-ptr |
| 17:06 | `4a46bde` 오타 수정 | benjohnbill | personal branch |
| 19:06, 19:53 | `d5f1779` `e223db2` "Final Result" | benjohnbill | **direct push to develop**: write unmapped buffer → exit(-1) |
| 19:54 | PR #117 develop → main, closed same minute | | abortive |
| 20:06 | PR #118 develop → main opened | | 94/96 at TIMEOUT=60; syn-read, multi-oom pass at TIMEOUT=600 |
| 20:08 | transcript 82ee74c4 | | Conflict: cause is #115 (main has `SYS_REMOVE`, develop has `SYS_CREATE`, same switch spot) |
| 20:09 | `dd1b4c8` | **copilot-swe-agent[bot]** (co-author benjohnbill) | GitHub Copilot resolves the conflict by merging origin/main into develop; nobody re-ran tests |
| 20:13 | PR #118 merged, `900aff9` | | final main |

## 2. Branch graph data (for a GitHub Flow diagram)

Lanes: `main`, `develop`, `benjohnbill`, `nuri`, `JSY-*`, temporary `integrate/*`, `tools/plan-check`.

```
main     7f88707─4a50cae─(#104 7465e75 10-15 20:55)─6 direct docs commits─1854f0b ─────────────(#115 af3c711 10-21 15:23)──(#118 900aff9 10-21 20:13)
                                                                   │                                     ▲  JSY-remove           ▲
develop                                                            └─(#105 10-17 22:21)─(#106,#107 10-19 10:55/10:59)─(#108 10-19 22:32)─806c8bb*─(#110 10-20 21:59)─15308de*─(#111 10:32)─c92cdac*─(#113 14:23)─(#112 14:24)─(#114 14:26)─(#116 15:22)─d5f1779*─e223db2*─dd1b4c8(main→develop, Copilot 20:09)
                                                                                                                                               (* = direct push, no PR)
```

Merges into develop (all GitHub "Merge pull request" commits unless noted):

| PR | Source branch | Merge commit | Merged (KST) | What came in |
|---|---|---|---|---|
| #105 | integrate/args-syscall (← JSY-argument-passing, nuri, benjohnbill) | `7ac9c04` | 10-17 22:21 | args (JSY), write/exit + wait loop (nuri), name line (benjohnbill) |
| #106 | tools/plan-check | `1334cec` | 10-19 10:55 | make pcheck |
| #107 | tools/plan-check | `714c06a` | 10-19 10:59 | make ptest |
| #108 | integrate/fork-wait-exec (← nuri, benjohnbill, JSY-wait-exec) | `aaea825` | 10-19 22:32 | fork + wait (nuri), exec (JSY) |
| #110 | integrate/nuri-fd (← nuri) | `daf0f23` | 10-20 21:59 | fd base, extract_thread_name |
| #111 | nuri | `4cb685a` | 10-21 10:32 | open/close/read, fork fd dup |
| #113 | benjohnbill | `dffb3e5` | 10-21 14:23 | exit fd cleanup, read W-bit, filesize, tell |
| #112 | nuri | `052a71f` | 10-21 14:24 | seek, close-all at exit |
| #114 | benjohnbill | `06adc15` | 10-21 14:26 | remove duplicate cleanup loop |
| #116 | nuri | `04e4df2` | 10-21 15:22 | create |
| — | origin/main (#115) | `dd1b4c8` | 10-21 20:09 | remove, via Copilot conflict fix |

Into main: #104 (10-15 20:55), #115 (10-21 15:23, mistake), #118 (10-21 20:13). Closed unmerged: #109, #117.
Pulls from develop into personal branches (merge commits): nuri `6cd1751` (10-19 16:59), `bddff87` (10-20 22:10); benjohnbill `864693e` (10-20 22:11), `fcd9eb0` (10-21 10:34), `afc64fa` (10-21 14:05). `origin/JSY-argument-passing` was later reset to `7ac9c04` (the #105 merge), so it carries no own work now.

Integration style: #105 and #108 used a throw-away `integrate/*` branch with one `git merge` per teammate, so every teammate's commits stay in history (stated in #105 body). From #111 on, personal branches PR'd into develop directly.

## 3. Test counts per merge

| PR | Before | After | Source |
|---|---|---|---|
| #104 | 18/95 | 18/95 (format only; disassembly diff 0) | #104 body |
| #105 | args ×5 + exit: 0/6 | 6/6; threads 18/18; `make lint` fails (5 JSY layout lines) | #105 body, judged with TIMEOUT=10 |
| #106/#107 | — | ptest: 95 results in 26 s, threads 18 pass | #107 body |
| #108 | not measured ("평가하지 말고") | build OK, `exec-once` FAIL | #108 body; 452d8bf4 |
| after #108 | — | board 11/21; 46/95 pass (49 fail) | cec9e2f2 23:13 |
| after `806c8bb` | 11/21 | 15/21; 50/95 | cec9e2f2 23:27; #38918 |
| #110 | 50/95, lint 8 layout lines, 0 name | same 50 PASS, 0 name violations | #110 body |
| #111, #112 | "확인 필요" | "확인 필요" | bodies (no numbers) |
| #113 | 26/95 FAIL (develop `c92cdac`) | 23/96 FAIL; new pass read-normal, multi-child-fd, multi-recurse, read-code-seg | #113 body |
| #114 | 4 named tests pass | same 4 + open-normal, open-twice, close-normal | #114 body (TIMEOUT=30) |
| #115 | not run ("전체 make check 미실행") | syn-remove blocked on create | #115 body |
| #116 | — | all pass except multi-oom, write-bad-ptr (incl. bad-* ×6) | #116 body |
| #118 | main has no `SYS_CREATE` | 94/96 at TIMEOUT=60; syn-read + multi-oom pass at TIMEOUT=600 | #118 body; 82ee74c4 20:06 |

96 = 95 upstream + the team-added `read-code-seg` (#113).

## 4. Merge criteria and measurement tables

### 4a. The Monday meeting card (plan) — `notes/team-plan.json`, block `mon-meet`

> 월 저녁 · 회의: 프로세스 3버전 비교 → develop
> - 브랜치별 측정: lint · 프로세스 테스트 14개 · threads 회귀 18개
> - 베이스 선택 → 좋은 부분 합치기 → develop PR
> - 포인터 검증 = benjohnbill 버전
> - fd 테이블 초안 설명 10분

Other plan blocks: `sat-proc` (주말, benjohnbill: fork·wait·exec + 포인터 검증, "월 저녁 회의 전까지 push 없음"), `mon-proc` (월, Hannuri-Song + JSZeroDev: fork·wait·exec 각자, 14 tests), `mon-file` (월, benjohnbill: fd 테이블 + 파일 syscall), `tue-fd` (nuri: fork fd dup), `tue-rox` (JSZeroDev: rox), `tue-base` (benjohnbill: filesys/base 13), `wed` (여유, multi-oom, threads 18, 95 all).

### 4b. What actually happened Monday night — no per-branch measurement

Transcript 452d8bf4, 10-19 22:19: "1. Nuri의 fork하고 2. benjohnbill의 wait하고 3. JSYZerodev의 exec … 머지만 해주라. 그냥, 따로 평가하지 말고."
- benjohnbill's wait could not be kept: nuri's fork is built on nuri's `child_status`. At 22:22 he chose "그냥 nuri의 wait으로 가줘. 호환성 문제 생각하면."
- JSY's branch also had its own wait (`child_info`); only `SYS_EXEC` was taken.
- No lint/14/18 table exists for #108 in PR body, transcripts, notes or claude-mem. Only a build check was run. The 21-test measurement came after the merge (cec9e2f2).

### 4c. The measured, criteria-based merge was Saturday #105 (transcript 7d7fd804)

Criteria he gave (10-17 21:28): 1. CONVENTION.md 충실도 2. stable 함 3. 시간/공간 복잡도 4. 코드 가독성.
Measurement table Claude produced (TIMEOUT=10 cut, real `.ck` judging, `main` as control):

| | args ×5 | exit | `make lint` | threads regression (3 run) |
|---|---|---|---|---|
| benjohnbill | 5/5 | PASS | clean | priority-donate-one FAIL |
| nuri | 5/5 | PASS | clean | 3/3 |
| JSY | 1/5 on a copy with syscalls added (name bug) | PASS | layout 5 lines | not run |

Per-criterion verdict: convention (JSY marker comments + lint; nuri C6 magic numbers, C4 `size`; benjohnbill C6 `8`), stability (only JSY checks the one-page stack bound; only nuri validates the write buffer), complexity (JSY malloc by token count; nuri 1040 B + 4 KB on kernel stack), readability benjohnbill > JSY > nuri.
This session also produced the `jungle-team-merge` skill (sequence: facts → per-branch measurement → per-criterion evaluation → plan → gate A → merge → verify → PR → gate B), used again for #110 (4a6080d9, 10-20 21:39).

### 4d. CONVENTION.md as AI evaluation item — confirmed

- Criterion 1 of #105 is "CONVENTION.md 충실도"; C3/C4/C6 rule ids are cited in the verdict.
- #110: integration commit `899bb3c` renames `struct file_descriptor`→`struct fd_entry` (C4) and `process_get_file`→`process_find_file` (C2); body "lint 이름 0건".
- nuri's own PR #111 lists "FD 관련 변수명 및 구조를 프로젝트 컨벤션에 맞게 정리".

## 5. `main` mistakes

1. **PR #115 `JSY-remove` → main**, merged by benjohnbill 10-21 15:23 (`af3c711`). Base should have been develop. Result: main had `SYS_REMOVE`, develop did not.
2. That made **#118 conflict** in `syscall.c`. The conflict was resolved at 20:09 by **copilot-swe-agent[bot]** (`dd1b4c8`, "Copilot started work on behalf of benjohnbill" in the GitHub UI). Claude (82ee74c4 20:11) had warned that "Use Both" in Zed would merge the two near-identical case bodies wrongly; the Copilot result kept both cases intact, but no tests were run before the 20:13 merge.
3. **PR #109** `integrate/fork-wait-exec` → main, 10-19 22:57, empty template, closed in the same minute.
4. **PR #117** develop → main, 10-21 19:54, empty template, closed in the same minute; replaced by #118 at 20:06.
5. Not a mistake but against "main only at start/end": 6 direct pushes to main on 10-15 after #104 (docs/lint polish). They were before develop existed.
Related: 5 direct pushes to **develop** without a PR — `806c8bb` (JSY 10-19), `15308de` (benjohnbill 10-20), `c92cdac` (JSY 10-21), `d5f1779` and `e223db2` (benjohnbill 10-21).

## 6. Who authored what survives (git blame -w on `origin/main`, ignore-revs on)

`syscall.c` (P2 lines): Hannuri Song ≈250 (write/exit, open/close, read, seek, create, wait/fork cases), JSZeroDev ≈91 (exec 49, remove 33, rox 9), benjohnbill ≈58 (tell 15, filesize 13, exit message 12, read W-bit 10, write patch 6+2), Copilot 1.
`process.c` (P2 lines): Hannuri Song ≈388 (fork 117, wait 96, fd base 85, thread name 38, fd dup 32, close-all 18), JSZeroDev ≈101 (arg passing 80, rox 19, name 2), benjohnbill ≈16 (exit message in `process_exit` 10, CONVENTION renames 6).
`exception.c`: benjohnbill 5 lines (`15308de`, user page fault → exit(-1)).
`thread.h`/`thread.c`: nuri 8+12, JSY 1+1, benjohnbill 1.

Rough picture: arg passing = JSY; fork/wait/fd/file syscalls = nuri; exec, remove, rox = JSY; benjohnbill's surviving code = exit message placement (`e37bea7`), page-fault exit(-1), filesize/tell, read W-bit check + `read-code-seg` test, write unmapped → exit(-1), naming fixes. Plus all tooling (#104 convention/lint, #106/#107 pcheck/ptest) and every integration.
His own words, 10-20 night (rewrite session): "fork는 내가 짠게 아니기도 하고, exec도 …".

## 7. Student claims → verdict

| Claim | Verdict | Evidence |
|---|---|---|
| GitHub Flow style: personal branches → develop after daily meeting/merge | **Confirmed, with nuance.** Daily integrations: 10-17 #105, 10-19 #108, 10-20 #110, 10-21 #111–#116. Also 5 direct pushes to develop without PR. | §1, §2 |
| main touched only at start and end | **Mostly confirmed.** Start: 10-15 import + #104 + 6 direct docs commits. End: #118. Exceptions = #115 merged into main by mistake; #109, #117 opened to main and closed. | §5 |
| fork/wait/exec due Monday 21:00 core time | **Partly confirmed.** Team notice "fork/wait/exec를 월요일까지" (545787f1 10-17 23:19); plan: Monday evening comparison meeting. He himself aimed exec "오늘 저녁" and fork "저녁 9시까지 … 희망사항" (3f6b74d6 18:42). JSY 21:15, nuri 21:16 commits land right at 21:00; merge at 22:19. The words "21:00 core time" and "force merge" appear in no transcript or note; only in his 10-22 account (claude-mem #39566 echoes it). | §1 Mon |
| Whoever not done gets force-merged; result fork+wait nuri, exec JSY | **Confirmed result.** #108 body table. The one not done was benjohnbill himself (only wait finished at 17:05; exec stage 1). He wanted his wait kept; it lost because nuri's fork depends on nuri's `child_status`. | 452d8bf4, #108 |
| His code survived only as some exception handling and exit/write etc. ("거의 다 merge 당함") | **Confirmed, slightly understated.** Surviving: exit message in `process_exit`, page fault exit(-1), write unmapped → exit(-1), read W-bit + added test, filesize, tell, renames. `write` handler body itself is nuri's. | §6 |
| Merge priority by 3 criteria (lint · 14 process tests · 18 threads), done with AI | **Corrected.** The 3 measures are on the Monday card (team-plan.json `mon-meet`), but #108 was merged "따로 평가하지 말고" with no measurement. The criteria-based, AI-measured merge was **#105 on Saturday** with 4 criteria (convention, stability, complexity, readability) and lint/args/exit/threads measurements. | §4 |
| CONVENTION.md made naming consistent; also an AI evaluation item | **Confirmed.** #105 criterion 1; #110 `899bb3c` renames; nuri #111 notes convention cleanup. | §4d |
| Split phase Tue–Wed, each took syscalls | **Confirmed, mostly Wed.** Tue: nuri fd base (#110) + open/close/read; Wed: nuri seek/create/fd dup, JSY rox/remove, benjohnbill exit cleanup/filesize/tell/read check. Matches plan `tue-*` blocks loosely (benjohnbill's filesys/base block was not done as such). | §1 |
| Merged per feature with separate merge commits | **Confirmed.** 6 PR merge commits on 10-21 (#111–#116), each one feature group (#111 bundles open/close/read + fork dup). | §2 |
| Compared test results against the AI-predicted set | **Partly confirmed.** benjohnbill's PRs carry before/after and "새로 pass" lists (#110 predicted "새로 통과할 테스트 없음", #113, #114). Nuri's #111/#112 say "확인 필요"; #115 not run; #116 gives a pass/fail summary. | §3 |
| Most active communication Tue–Wed | **Unknown from git.** 10-21 has 6 PRs in 5 h (10:29–15:22) and 3 people pushing; no chat logs in repo. | §1 |
| All tests passed Wednesday evening | **Confirmed with caveat.** #118 (20:06): 94/96 at TIMEOUT=60; syn-read and multi-oom pass only at TIMEOUT=600. syn-read failed by `open "data": FAILED` (possible missing filesys lock), not timeout. Final conflict fix by Copilot was merged untested. | §3, §5 |
