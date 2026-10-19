# Team convention checks. Run from the repository root.
#   make format   rewrite the C files in the team style (.clang-format)
#   make lint     report style and naming violations; fail (nonzero exit) if any
# The editor shows the same results live through clangd.
# Pintos tests are upstream files; `make check` in pintos/userprog runs them.
#   make pcheck WHO=<name>   after a test run, sync that person's checks on the
#                            team plan page (PASS checks, FAIL clears); DRY=1 only prints

# The file lists and output formatting live in scripts/convention.py.
format:
	@python3 scripts/convention.py format

lint:
	@python3 scripts/convention.py lint

# scripts/plan-check.py reads pintos/userprog/build/tests/**/*.result.
pcheck:
	@python3 scripts/plan-check.py --who "$(WHO)" $(if $(DRY),--dry-run)

.PHONY: format lint pcheck
