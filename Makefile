
CFLAGS ?= -O2 -Wall -pedantic

kill_fsync.so: kill_fsync.o
	gcc -shared -Wl,-soname,$@ -o $@ -ldl $<

kill_fsync.s: kill_fsync.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -S -o $@ $<
