# Team convention checks. Run from the repository root.
#   make format   rewrite the C files in the team style (.clang-format)
#   make lint     report style and naming violations; fail (nonzero exit) if any
# The editor shows the same results live through clangd.
# Pintos tests are upstream files; `make check` in pintos/userprog runs them.
#   make ptest    inside the container: run every test fast, then pcheck
#   make pcheck   sync your checks on the team plan page with the last results
#                 (PASS checks, FAIL clears); DRY=1 only prints
# Both take your name from `git config pintos.who <name>` (set it once) or WHO=.

# The file lists and output formatting live in scripts/convention.py.
format:
	@python3 scripts/convention.py format

lint:
	@python3 scripts/convention.py lint

# scripts/plan-check.py reads pintos/userprog/build/tests/**/*.result.
pcheck:
	@python3 scripts/plan-check.py $(if $(WHO),--who "$(WHO)") $(if $(DRY),--dry-run)

# ptest: clear the old results (check skips a test whose output is newer than
# the kernel), run all tests in parallel, then pcheck even if some FAIL.
#   icount + sleep=off counts guest time in instructions, so parallel load does
#   not shake the timing tests (shift 0-2 time out). TIMEOUT is wall-clock
#   seconds; a slow machine may need TIMEOUT=10 or more against false FAILs.
#   PATH: the pintos runner, without sourcing pintos/activate first.
JOBS    ?= $(shell nproc)
SHIFT   ?= 5
TIMEOUT ?= 5
ptest:
	@command -v qemu-system-x86_64 >/dev/null || \
	  { echo 'ptest: run it inside the container (DevContainer terminal)'; exit 1; }
	@find pintos/userprog/build/tests \( -name '*.output' -o -name '*.errors' \
	  -o -name '*.result' \) -delete 2>/dev/null; true
	-@PATH="$(CURDIR)/pintos/utils:$$PATH" PINTOS_QEMU_EXTRA="-icount shift=$(SHIFT),sleep=off" \
	  $(MAKE) --no-print-directory -s -C pintos/userprog -j$(JOBS) check TIMEOUT=$(TIMEOUT)
	@$(MAKE) --no-print-directory pcheck

.PHONY: format lint pcheck ptest
