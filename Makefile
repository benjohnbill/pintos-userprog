# Team convention checks. Run from the repository root.
#   make format   rewrite the C files in the team style (.clang-format)
#   make lint     report style and naming violations; exit 1 if any
# The editor shows the same results live through clangd.
# Pintos tests are upstream files; `make check` in pintos/userprog runs them.

# Every tracked C file except the upstream tests and tools.
FORMAT_FILES = $(shell git ls-files -- 'pintos/*.c' 'pintos/*.h' \
	':!pintos/tests/*' ':!pintos/utils/*')

# Naming checks cover the kernel parts that the userprog build compiles.
# lib/ is the C library (standard names such as PRId64) and vm/ is not built
# until Project 3, so both are left out.
LINT_DIRS = threads devices userprog filesys
LINT_FILES = $(shell git ls-files -- $(foreach d,$(LINT_DIRS),'pintos/$(d)/*.c'))
LINT_HEADERS = pintos/(include/)?($(subst $() ,|,$(LINT_DIRS)))/

format:
	@clang-format -i $(FORMAT_FILES)

# clang 15+ makes int-conversion an error; gcc 11 (the real build) warns.
# Demoting it keeps an upstream line in threads/mmu.c from blocking lint.
# Both checks run even when the first fails, so one run shows everything.
lint:
	@rc=0; \
	clang-format --dry-run --Werror $(FORMAT_FILES) || rc=1; \
	clang-tidy --quiet --warnings-as-errors='*' \
	  --header-filter='$(LINT_HEADERS)' \
	  --extra-arg=-Wno-error=int-conversion $(LINT_FILES) || rc=1; \
	exit $$rc

.PHONY: format lint
