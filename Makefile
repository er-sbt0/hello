CC = gcc
CFLAGS = -Wall -Wextra -O2 -Isrc -I.
TARGET = hello

SRCS = $(sort $(wildcard *.c) $(wildcard src/*/*.c))
OBJS = $(SRCS:.c=.o)
DEPS = $(OBJS:.o=.d)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

run: $(TARGET)
	./$(TARGET)

# Regenerate the compilation database Sourcetrail indexes from.
compdb: compile_commands.json

compile_commands.json: $(SRCS) Makefile
	@printf '[\n' > $@
	@sep=""; for src in $(SRCS); do \
		obj=$${src%.c}.o; \
		printf '%s {\n  "directory": "%s",\n  "arguments": ["%s", "-Wall", "-Wextra", "-O2", "-Isrc", "-I.", "-c", "%s", "-o", "%s"],\n  "file": "%s"\n }' \
			"$$sep" "$(CURDIR)" "$(CC)" "$$src" "$$obj" "$$src" >> $@; \
		sep=","; \
	done
	@printf '\n]\n' >> $@
	@echo "wrote $@ with $(words $(SRCS)) entries"

# Regenerate the module-graph diagram (TypeScript, run directly by node >= 22).
diagram: docs/architecture.svg

docs/architecture.svg: tools/arch-diagram.ts $(SRCS) $(wildcard *.h) $(wildcard src/*/*.h)
	node tools/arch-diagram.ts $@

clean:
	rm -f $(TARGET) $(OBJS) $(DEPS)

distclean: clean
	rm -f compile_commands.json

.PHONY: all run compdb diagram clean distclean

-include $(DEPS)
