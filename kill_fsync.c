#define _GNU_SOURCE
#include <dlfcn.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>
#include <inttypes.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

__attribute__ ((section(".text.fsync_kill_patch_instructions")))
static
int patch_instructions()
{
	return 0;
}

__attribute__ ((section (".text.fsync_kill_patch_instructions")))
static
void patch_instructions_end_marker() {}

__attribute__ ((constructor))
static
void do_kill_fsync(void)
{
	intptr_t fsync_address = (intptr_t)(dlsym(RTLD_DEFAULT, "fsync"));
	intptr_t fdatasync_address = (intptr_t)(dlsym(RTLD_DEFAULT, "fdatasync"));
	long page_size = sysconf(_SC_PAGE_SIZE);
	size_t patch_instructions_size;
	Dl_info dlinfo;
	int rv;

	rv = mprotect((void *)(fsync_address & ~(page_size - 1)), page_size, PROT_READ|PROT_WRITE|PROT_EXEC);
	if (rv) {
		perror("mprotect(fsync_address)");
		abort();
	}
	rv = mprotect((void *)(fdatasync_address & ~(page_size - 1)), page_size, PROT_READ|PROT_WRITE|PROT_EXEC);
	if (rv) {
		perror("mprotect(fdatasync_address)");
		abort();
	}

	patch_instructions_size = (char *)(intptr_t)patch_instructions_end_marker - (char *)(intptr_t)patch_instructions;
	if (patch_instructions_size > 16) {
		fprintf(stderr, "patch_instructions_size is too big");
		abort();
	}

	rv = dladdr((void *)(fsync_address + patch_instructions_size - 1), &dlinfo);
	if (!rv) {
		perror("dladdr(fsync_address)");
		abort();
	}

	if (strcmp(dlinfo.dli_sname, "fsync")) {
		fprintf(stderr, "patch_instructions_size is too big for fsync code\n");
		abort();
	}
	assert((intptr_t)dlinfo.dli_saddr == fsync_address);

	rv = dladdr((void *)(fdatasync_address + patch_instructions_size - 1), &dlinfo);
	if (!rv) {
		perror("dladdr(fdatasync_address)");
		abort();
	}

	if (strcmp(dlinfo.dli_sname, "fdatasync")) {
		fprintf(stderr, "patch_instructions_size is too big for fdatasync code\n");
		abort();
	}
	assert((intptr_t)dlinfo.dli_saddr == fdatasync_address);

	memcpy((char *)fsync_address, (void *)(intptr_t)patch_instructions, patch_instructions_size);
	memcpy((char *)fdatasync_address, (void *)(intptr_t)patch_instructions, patch_instructions_size);
}
