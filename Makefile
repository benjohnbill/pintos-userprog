# Team convention checks. Run from the repository root.
#   make format   rewrite the C files in the team style (.clang-format)
#   make lint     report style and naming violations; fail (nonzero exit) if any
# The editor shows the same results live through clangd.
# Pintos tests are upstream files; `make check` in pintos/userprog runs them.

# The file lists and output formatting live in scripts/convention.py.
format:
	@python3 scripts/convention.py format

lint:
	@python3 scripts/convention.py lint

.PHONY: format lint
