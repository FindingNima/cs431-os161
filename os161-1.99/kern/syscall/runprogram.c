/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2008, 2009
 *	The President and Fellows of Harvard College.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Sample/test code for running a user program.  You can use this for
 * reference when implementing the execv() system call. Remember though
 * that execv() needs to do more than this function does.
 */

#include <types.h>
#include <kern/errno.h>
#include <kern/fcntl.h>
#include <lib.h>
#include <proc.h>
#include <current.h>
#include <addrspace.h>
#include <vm.h>
#include <vfs.h>
#include <syscall.h>
#include <test.h>
#include <copyinout.h>
#include <opt-A2.h>

/*
 * Load program "progname" and start running it in usermode.
 * Does not return except on error.
 *
 * Calls vfs_open on progname and thus may destroy it.
 */

#if OPT_A2
int runprogram(char *progname, char **args)
{
#else
int runprogram(char *progname)
{
#endif
	struct addrspace *as;
	struct vnode *v;
	vaddr_t entrypoint, stackptr;
	int result;

	/* Open the file. */
	result = vfs_open(progname, O_RDONLY, 0, &v);
	DEBUG(DB_EXEC, "result: %d\n", result);
	if (result) {
		return result;
	}

	/* We should be a new process. */
	KASSERT(curproc_getas() == NULL);

	/* Create a new address space. */
	as = as_create();
	if (as ==NULL) {
		vfs_close(v);
		return ENOMEM;
	}

	/* Switch to it and activate it. */
	curproc_setas(as);
	as_activate();

	/* Load the executable. */
	result = load_elf(v, &entrypoint);
	DEBUG(DB_EXEC, "entrypoint: %x\n", entrypoint);
	if (result) {
		/* p_addrspace will go away when curproc is destroyed */
		vfs_close(v);
		return result;
	}

	/* Done with the file now. */
	vfs_close(v);

	/* Define the user stack in the address space */
	result = as_define_stack(as, &stackptr);
	DEBUG(DB_EXEC, "stackptr: %x\n", stackptr);
	if (result) {
		/* p_addrspace will go away when curproc is destroyed */
		return result;
	}

#if OPT_A2
	/* keep track of how many arguments there are */
	int argc = 0;
	
	char **args_counter = args;
	
	while (*args_counter != NULL)
	{
		argc++;
		args_counter++;
	}

	/* create a new array that will hold argument lengths*/
	int arg_lengths[argc];

	int i;
	int length;
	args_counter--;
	char *arg_char = *args_counter;
	char **traversing_arg = args_counter;
	vaddr_t argv[argc];

	DEBUG(DB_EXEC, "argc: %d\n", argc);
	
	for (i = argc - 1; i >= 0; i--)
	{
		DEBUG(DB_EXEC, "ENTERING FOR LOOP\n");
		/* this is set to one to account for the terminating 0 value */
		length = 1;

		while (*arg_char != 0)
		{
			DEBUG(DB_EXEC, "arg char: %s\n", arg_char);
			length++;
			arg_char++;		
		}

		DEBUG(DB_EXEC, "ARGUMENT: %s\n", *traversing_arg);
		traversing_arg--;
		arg_char = *traversing_arg;
		arg_lengths[i] = length;
		DEBUG(DB_EXEC, "length: %d\n", length);

		/* copy string of arguments from kernal space to uer space*/
		size_t *actual;
		if ((length % 4) > 0)
		{
			length += (4 - (length % 4));
		}
		
		stackptr -= length;

		copyoutstr(*traversing_arg, (userptr_t)stackptr, length, actual);
		DEBUG(DB_EXEC, "string stack pointer: %x\n", stackptr);
		
		/* copy argument pointers from kernal space to user space */
		argv[i] = stackptr;
	}
 
//	stackptr -= (4 * (argc + 1));

	/* account for NULL terminator */
	stackptr -= 4;
	for (i = argc - 1; i >= 0; i--)
	{
		stackptr -= 4;
		copyout((const void*)argv[i], (userptr_t)stackptr, sizeof(char*));

		DEBUG(DB_EXEC, "pointer stack pointer: %x\n", stackptr);
		DEBUG(DB_EXEC, "argv[%d] = %x\n",i, argv[i]);
	}

//	copyout(argv, (userptr_t)stackptr, argc);

//	stackptr -= (4 * (argc + 1));
//	copyout(*args, (userptr_t)stackptr, argc);
//	DEBUG(DB_EXEC, "pointer stack pointer: %x\n", stackptr);
	
	enter_new_process(argc, (userptr_t)stackptr, stackptr, entrypoint);
#else
	/* Warp to user mode. */
	enter_new_process(0 /*argc*/, NULL /*userspace addr of argv*/, stackptr, entrypoint);
#endif
	/* enter_new_process does not return. */
	panic("enter_new_process returned\n");
	return EINVAL;
}

