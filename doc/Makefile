# Compiler definition. Ensure that the directory containing the 'mgc' executable is added to the system PATH.
# Alternatively, replace 'mgc' with the absolute path to the generated mgc.exe.
MGC = mgc

# Recursively find all .mgc files under the epic directory.
# Note: This relies on a Unix-like environment (e.g., Git Bash, MSYS2, MinGW, or WSL) for the 'find' command.
MGC_SRCS = $(shell find epic -type f -name "*.mgc")

# Generate paths for the target .tex files: map "epic/XXX.mgc" to "XXX.tex".
TEX_OBJS = $(patsubst epic/%.mgc, %.tex, $(MGC_SRCS))

.PHONY: all clean

# Default build target
all: $(TEX_OBJS)

# Compilation rule: depend on the corresponding .mgc file.
$(TEX_OBJS): %.tex: epic/%.mgc
	@echo "Compiling $< -> $@"
	@mkdir -p $(dir $@)
	$(MGC) $< -f tex -o $@

# Clean up the generated .tex files
clean:
	rm -f $(TEX_OBJS)
