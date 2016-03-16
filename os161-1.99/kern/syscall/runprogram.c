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
	if (result) {
		/* p_addrspace will go away when curproc is destroyed */
		vfs_close(v);
		return result;
	}

	/* Done with the file now. */
	vfs_close(v);

	/* Define the user stack in the address space */
	result = as_define_stack(as, &stackptr);
	if (result) {
		/* p_addrspace will go away when curproc is destroyed */
		return result;
	}

#if OPT_A2
 	/* keep track of how many arguments there are */
 	//DEBUG(DB_EXEC, "THIS SHOULD PRINT\n");
	int argc = 0;

	int i, j;
	int length;
	int actual_length;
	char *arg_buffer;


	// /* used for traversing through arguemnts */
	char **args_counter = args;
	
	while (*args_counter != NULL)
	{
		argc++;
		args_counter++;
	}

	// keeps track of stack pointers at begnning of each string argument
	char *arg_pointers[argc];


	// adding in strings backwards
	for (i = argc - 1; i >=0; i--)
	{
		length = strlen(args[i]);
		length ++; 	// accounts for \0 terminator
		actual_length = length;

		if (length % 4  != 0)
		{
			length += (4 - (length % 4));
		}

		// not including \0 terminator
		arg_buffer = kmalloc(sizeof(char) * (length - 1)); 

		for (j = 0; j < length; j++)
		{
			if(j >= actual_length)
				arg_buffer[j] = '\0';
			else
				arg_buffer[j] = args[i][j];

		}

		// moving the stack pointer
		stackptr -= length;

		// finally, let's copy out to the stack
		copyout((const void*)arg_buffer, (userptr_t)stackptr, (size_t)length);

		// keeps track of stack pointers at begnning of each string argument
		arg_pointers[i] = (char*)stackptr;

		// deallocate space
		kfree(arg_buffer);

	}

	for (i = argc - 1; i >=0; i--)
	{
		//  move the stack pointer down 4 bytes for the character pointer
		stackptr -= 4;
		copyout((const void*)(arg_pointers + i), (userptr_t)stackptr, 4);
	}

	enter_new_process(argc, (userptr_t)stackptr, stackptr, entrypoint);
	
#else
	/* Warp to user mode. */
	enter_new_process(0 /*argc*/, NULL /*userspace addr of argv*/, stackptr, entrypoint);
#endif
	/* enter_new_process does not return. */
	panic("enter_new_process returned\n");
	return EINVAL;
}
