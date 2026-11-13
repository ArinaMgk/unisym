t.me/phinaren

# UniSym Contribution Guidelines

## Collaboration

- Keep communication respectful and focused on technical matters. Critique code, design, or decisions, not people.
- When proposing a change, explain its purpose, expected behavior, and relevant limitations.
- Before proposing a convention or design change, check the existing code, documentation, and project rules.
- When project conventions are unclear, ask the maintainers instead of assuming based on personal preference or directory names.

## Git Workflow

- Fork the UniSym repository before contributing.
- Create a separate branch in your fork for independent changes. Do not work directly on the fork's default branch.
- Use a concise branch name that describes the change, such as `fix/elf-riscv-header-path` or `feature/aasm-riscv-relocations`.
- Open a Pull Request from your branch to the appropriate upstream branch. Do not push directly to the upstream repository's default branch.
- Before starting, check the current branch and working-tree status. Preserve existing changes; do not overwrite or discard work that is not yours.
- Before opening a Pull Request, inspect the diff and ensure it contains only the intended changes.
- Do not force-push, rewrite shared history, or merge another contributor's branch without maintainer approval.

## Naming

- **Macros:** `ABC`
- **Namespaces:** `a_b`
- **Classes:** `AaaBbb`
	- **Fields:** `a_b`
	- **Methods:** `(aaa)Bbb`, where `aaa` may be `en`, `get`, or `set`.

## Code / Text File

- Mixed line endings are not allowed. LF only or CRLF only is fine.
- Normally please use LF only.
- Use ASCII/UTF-8 encoding.

## C/C++

- Avoid including `stddef.h`, `stdint.h`, and `inttypes.h`. Prefer the project's established types and headers.
- Normally, `inc/` stores header files and `lib/` stores source files.
- Keep paths mirrored: a source file at `lib/<path>/<name>.cpp` normally has its corresponding project header at `inc/<path>/<name>.hpp`.
- Before adding or moving files, inspect nearby files and follow their directory and include conventions. Do not place headers under `lib/`.
- Keep changes focused. Do not reformat or rewrite unrelated code.
- Write code comments in English. Prefer a single-line comment for a simple point.

## Changes and Verification

- Add or update tests when behavior changes.
- Build the affected target and run relevant tests before reporting a change as complete.
- In the Pull Request, summarize the change and list the build and test commands run, including any failures or checks not run.
- Do not claim a change is verified when its build or tests have not passed.

## Existing Repository Changes

- Check the working tree before editing.
- Preserve staged, unstaged, and untracked work that predates your change.
- Do not reset, clean, overwrite, or revert existing work unless its owner explicitly requests it.
