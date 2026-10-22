# B. Test commands and the live plan board — fact sheet

Researched 2026-10-22. Sources: repo `week08` (git, Makefile, scripts), study site
`benjohnbill.github.io/study/week08/`, `gh` (read-only), claude-mem observations,
past Claude transcripts. One read-only GET of the Firebase checks node. No writes.

Calendar: Day 1 = 10-15 Thu · 10-16 Fri · 10-17 Sat · 10-18 Sun · 10-19 Mon · 10-20 Tue · 10-21 Wed. Times are KST.

## Claim check

| # | Claim | Verdict | Evidence |
|---|---|---|---|
| 1 | Built "on Day 2" | **Corrected.** Built over three steps, Day 3 to Day 5. | `dfast` (personal, local): 10-17 Sat 22:36 (obs #38530). Plan page: 10-18 01:53, Firebase sync 02:08 (site commits `555273d`, `690d113`). `make pcheck`: 10-19 Mon 10:40 (`1d9be04`, PR #106). `make ptest`: 10-19 10:53 (`e3b69f6`, PR #107). |
| 2 | Team commands `make ptest`, `make dptest` family | **Partly corrected.** Two layers. | Team, committed root `Makefile`: `format`, `lint`, `pcheck`, `ptest` (`Makefile:12-40`). Personal, local-only `GNUmakefile` (listed in `.git/info/exclude:20`): `dcheck`, `dfast`, `dptest`, `dr_<test>`, `dg_<test>`, `dshell`, `dimage` (`GNUmakefile:112-142`). Teammates got `ptest`/`pcheck` only. `dptest` = run the team `ptest` in the `pintosdev` container from the host (`GNUmakefile:136-140`). He asked for it 10-19 ~10:49: "나는 dptest를 쓸거 같고". |
| 3 | icount/shift | **Confirmed.** | `PINTOS_QEMU_EXTRA="-icount shift=$(SHIFT),sleep=off"`, `SHIFT ?= 5` (`Makefile:29,36`). The hook in `pintos/utils/pintos:137-140` came with the P1 baseline import (`7f88707`, 10-15 19:27). It is not new in P2. Shift 0–2 time out; use 3–8 to vary the timing (`README.md:23-24`). |
| 4 | Parallel QEMUs (-j?) | **Confirmed.** `-j$(JOBS)`, `JOBS ?= $(shell nproc)` | `Makefile:28,37`. His machine has 22 cores, so `-j22` (obs #38530; `make help` text "-j22"). `GNUmakefile:120-121`: run 3 trees at once → `JOBS=7` each, or too many QEMUs give false TIMEOUT FAILs. |
| 5 | TIMEOUT 5 s | **Confirmed.** Wall-clock seconds. | `TIMEOUT ?= 5` (`Makefile:30`). Upstream default is 60 (`pintos/tests/Make.tests:17`). Slow laptop: `make ptest TIMEOUT=10` (PR #107 body). Known cost: `multi-recurse` times out in parallel; final judgement by `make dcheck` (`notes/p2-debug.md:6`). |
| 6 | Results reflect on the 분업표/calendar via a separate Firebase server | **Confirmed, with nuance.** No own server. | Firebase Realtime Database, project `pintos-userprog`, region asia-southeast1, path `/team-plan/checks` (`scripts/plan-check.py:29-30`, `team-plan.html:143`). Plain REST (`PUT …/<key>.json` body `true`, or `DELETE`). No SDK, no auth token. Rules accept only `/team-plan/checks/<who|block|item> = boolean` (page comment `team-plan.html:141-142`; obs #38590). The page is a people × weekday table (사람 × 요일), not a real calendar. |
| 7 | "Live" | **Nuanced.** Near-live: the page polls every 10 s. | `setInterval(pull, 10000)` (`team-plan.html:232`). A click writes at once (`push()`). If Firebase is unreachable, the page falls back to localStorage and shows "공유 서버 연결 끊김 → 체크는 이 브라우저에만 저장" (`team-plan.html:150-153`). |
| 8 | `make pcheck WHO=<name>` | **Confirmed.** Later made optional. | PR #106 needed `WHO=`. PR #107 made it default to `git config pintos.who` (`plan-check.py:90-92`). Options: `DRY=1` (print only). Valid names come from the plan JSON: benjohnbill, Hannuri-Song, JSZeroDev. |
| 9 | "체크 = 팀 전원에게 공유 · 10초마다 갱신 · 팀 공용 진척은 Board 6" | **Confirmed verbatim.** | `team-plan.html:121`. "Board 6" links to `https://github.com/users/benjohnbill/projects/6`. |
| 10 | Board 6 | **Confirmed: GitHub Project "Week 8".** | `gh project list`: #6 "Week 8", 103 items. Fields: Status (Todo / In Progress / Done). Now all 103 are Done. Test issues `[WEEK8] 테스트 - <name>`: #8–#103 (96 issues), all opened Day 1, 10-15 ~19:50. Closed 57 on 10-20, 39 on 10-21. The plan JSON `issue` map links 71 tests to #13–#83 (`team-plan.json`). Each test row on the page shows its `#issue` link. |
| 11 | Makefile/Dockerfile, `pintosdev` container, host gcc 15 cannot build | **Confirmed.** | `.devcontainer/Dockerfile`: `ubuntu:22.04`, gcc, `qemu-system-x86` (gcc 11.4, QEMU 6.2 per `README.md:8`). `make dimage` builds the image as `pintosdev` (`GNUmakefile:102,109`). `ptest` refuses to run without `qemu-system-x86_64` ("run it inside the container") (`Makefile:32-33`). gcc 15 rule: `week08/CLAUDE.md` § Build and git. |
| 12 | Teammates used it | **Unknown.** | No teammate commit touches `Makefile` or `scripts/` (all 4 tooling commits are benjohnbill). Firebase today: all 106 boxes checked (benjohnbill 63, Hannuri-Song 18, JSZeroDev 17, shared 8). A check does not record who wrote it or whether a click or `pcheck` wrote it. Ask the teammates. |

## How it works (one slide)

```
make ptest  (or make dptest from the host)
  1. delete old *.output *.errors *.result      ← `check` skips a test whose output is newer than the kernel
  2. make -C pintos/userprog -j$(nproc) check TIMEOUT=5
       each QEMU: -icount shift=5,sleep=off      ← guest time counts instructions → parallel load cannot shake timing
  3. → pintos/userprog/build/tests/**/<test>.result   (first line PASS / FAIL)
  4. make pcheck → scripts/plan-check.py
       read plan   team-plan.json from the study site (same file the page draws)
       read results  skip mlfqs and any .result older than kernel.bin
       per block of WHO + shared blocks:  PASS → check · FAIL → clear · no result → leave
       auto rows: "threads 회귀 18개", "dfast 95개 전체 통과" = every result in that dir present and PASS
       PUT true / DELETE only the changed keys   who|block|item
  5. Firebase RTDB /team-plan/checks
  6. team-plan.html polls every 10 s → bars and "n / m" counts update for all 3
```

Design notes from PR #106/#107:
- One run syncs all of that person's blocks, so a Monday test that passes on Tuesday also checks the Monday block.
- Checks follow the branch you ran on. Running on a branch with fewer features clears checks that passed elsewhere. Run on your own work branch.
- No merge needed to try it: `git show origin/develop:scripts/plan-check.py | python3 - --who <name>`.

## Numbers

| What | Number | Source |
|---|---|---|
| icount `sleep=off` speed-up | 17–25×; `mlfqs-load-1` 53 s → 2 s | `README.md:23` (measured on the P1 baseline) |
| Before: sequential `dcheck` while `wait` was a busy loop | every userprog test runs to the 60 s TIMEOUT → 64 userprog tests take over 1 hour | transcript 10-17 (agent explanation); `Make.tests:17` |
| First `dfast`, 95 tests, `-j22`, TIMEOUT 5 | 23 s (36 PASS, 77 FAIL — `wait` not yet done) | transcript 10-17 13:37Z; obs #38531 |
| `ptest` on `develop` | 26 s, 95 `.result` files, threads 18/18 | PR #107 Evidence |
| Full 95 on `develop` (team-merge baseline) | 32 s | transcript 10-19 |
| Sequential `dcheck` with icount | 2 min 11.66 s | transcript 10-20 04:43Z |
| Plan page | 9 blocks, 106 check items, 71 test → issue links | `team-plan.json`; obs #38596 |

## Corrections to the claude-mem record

- Obs #38534 says `dfast` went into a "tracked, shared CLAUDE.md". Wrong: `CLAUDE.md` and `GNUmakefile` are both local-only (`.git/info/exclude`). `dfast` never reached teammates. `ptest` (PR #107) was the team version.
- Obs #38590 says the rules are "read-only". The page and `pcheck` both write. The rules restrict the key format and the value (boolean), not writing.
- Obs #38531 "estimated 1+ hours sequential" is an estimate from the 60 s × 64 tests arithmetic, not a measured run.

## Commits and PRs

| Ref | Date (KST) | Author | What |
|---|---|---|---|
| site `555273d` | 10-18 01:53 | benjohnbill | 분업표 page (사람 × 요일) |
| site `690d113` | 10-18 02:08 | benjohnbill | checks shared through Firebase |
| site `8c6c809` | 10-19 10:39 | benjohnbill | plan data split out to `team-plan.json` |
| `1d9be04` / PR #106 | 10-19 10:40, merged 10:55 | benjohnbill | `make pcheck` + `scripts/plan-check.py` (branch `tools/plan-check` → `develop`) |
| `e3b69f6` / PR #107 | 10-19 10:53, merged 10:59 | benjohnbill | `make ptest`; WHO from `git config pintos.who`; PATH fix (Error 127 before) |
