/*
 * Copyright (c) 2025 0l3d
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
// -----------------------------------
/*
 * I wrote this code for all POSIX systems, but it has different syscall handling.
 * The main idea is to make a language that is as flexible as possible.
 * In the future, we can write a Windows version.
 * This code is still in testing.
 */
#include <stdarg.h>
#include <stdint.h>
#include <unistd.h>

typedef long    (*syscall_port_func) (long r0, long r1, long r2, long r3, long r4, long r5);

struct syscall_entry {
	const char     *name;
	syscall_port_func pf;
};

enum {
	SYS_READ = 0,
	SYS_WRITE = 1,
	SYS_MAX
};



long
sys_read(long fd, long buf, long len, long unused_r4, long unused_r5)
{
	return read(
		    (int) fd,
		    (void *) buf,
		    (size_t) len
	);
}

long
sys_write(long fd, long buf, long len, long unused_r4, long unused_r5)
{
	return write(
		     (int) fd,
		     (void *) buf,
		     (size_t) len
	);
}

static struct syscall_entry
		syscall_table [SYS_MAX] = {
	[SYS_READ] = {"read", sys_read},
	[SYS_WRITE] = {"write", sys_write},
};

long
e_syscall(long number, long r0, long r1, long r2, long r3, long r4, long r5)
{
	if (number < 0 || number >= SYS_MAX)
		return -1;

	syscall_port_func fn = syscall_table[number].pf;
	if (!fn)
		return -1;

	return fn(r0, r1, r2, r3, r4, r5);
}
