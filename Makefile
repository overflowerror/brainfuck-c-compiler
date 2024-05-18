CC       = gcc
CFLAGS   = -std=c11 -D_POSIX_C_SOURCE=200809L -Wall -Wpedantic -g
LD       = gcc

OBJS     = obj/main.o
DEPS     = $(OBJS:%.o=%.d)

-include $(DEPS)

all: bfc test

obj/%.o: src/%.c obj
	$(CC) $(CFLAGS) -c -o $@ $<

bfc: $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $^

test: bfc FORCE
	@echo "No tests yet."

FORCE: ;

clean:
	@echo "Cleaning up"
	@rm -f obj/*.o
	@rm -f obj/*.d
	@rm bfc

