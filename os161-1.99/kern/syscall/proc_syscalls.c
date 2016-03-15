#include <types.h>
#include <kern/errno.h>
#include <kern/unistd.h>
#include <kern/wait.h>
#include <lib.h>
#include <syscall.h>
#include <current.h>
#include <proc.h>
#include <thread.h>
#include <addrspace.h>
#include <copyinout.h>
#include "opt-A2.h"

  /* this implementation of sys__exit does not do anything with the exit code */
  /* this needs to be fixed to get exit() and waitpid() working properly */

void sys__exit(int exitcode) {

  struct addrspace *as;
  struct proc *p = curproc;
  /* for now, just include this to keep the compiler from complaining about
     an unused variable */
#if OPT_A2
  // set the exit code of the proc
  curproc->p_exitcode = exitcode;
  // increment the binary semaphore for the process
  V(&curproc->sem_running);
  // should now wait till all processes that are waitpid are notified
  while(curproc->sem_waiting->sem_count != 0)
  {
    // wait
  }

#else
  (void) exitcode
#endif

  DEBUG(DB_SYSCALL,"Syscall: _exit(%d)\n",exitcode);

  KASSERT(curproc->p_addrspace != NULL);
  as_deactivate();
  /*
   * clear p_addrspace before calling as_destroy. Otherwise if
   * as_destroy sleeps (which is quite possible) when we
   * come back we'll be calling as_activate on a
   * half-destroyed address space. This tends to be
   * messily fatal.
   */
  as = curproc_setas(NULL);
  as_destroy(as);

  /* detach this thread from its process */
  /* note: curproc cannot be used after this call */
  


  proc_remthread(curthread);

  /* if this is the last user process in the system, proc_destroy()
     will wake up the kernel menu thread */
  proc_destroy(p);
  
  thread_exit();
  /* thread_exit() does not return, so we should never get here */
  panic("return from thread_exit in sys_exit\n");
}


/* stub handler for getpid() system call                */
int
sys_getpid(pid_t *retval)
{
  /* for now, this is just a stub that always returns a PID of 1 */
  /* you need to fix this to make it work properly */
#if OPT_A2
  return curproc->pid;
#else
  (void) retval;
  return 0;
#endif
}

/* stub handler for waitpid() system call                */

int
sys_waitpid(pid_t pid,
      userptr_t status,
      int options,
      pid_t *retval)
{
  int exitstatus;
  int result;

  /* this is just a stub implementation that always reports an
     exit status of 0, regardless of the actual exit status of
     the specified process.   
     In fact, this will return 0 even if the specified process
     is still running, and even if it never existed in the first place.
     Fix this!
  */

  if (options != 0) {
    return(EINVAL);
  }
#if OPT_A2
  struct proc* reference_proc = pids[pid];
  // you should get the referenced process from th pid by making a table each time a proc is created
  V(&reference_proc-sem_waiting);
  P(&reference_proc->sem_running);

  exitstatus = reference_proc->exitcode;
  V(&reference_proc->sem_running);
  P(&reference_proc->sem_waiting);
  result = copyout((void *)&exitstatus,status,sizeof(int));
  if (result) {
    return(result);
  }
  *retval = pid;
  return exitstatus;
#else
  /* for now, just pretend the exitstatus is 0 */
  exitstatus = 0;
  result = copyout((void *)&exitstatus,status,sizeof(int));
  if (result) {
    return(result);
  }
  *retval = pid;
  return(0);
#endif
}

#if OPT_A2
pid_t fork(){
 struct addrspace *as;
 vaddr_t stackptr;
 int result;
 struct proc *child;

 //Create a new process
 child = proc_create(curproc->p_name);
 
 
 /* Create a new address space. */
 as = as_create();
 if (as ==NULL) {
   vfs_close(v);
   return ENOMEM;
 }
 //Set address space of new process
 spinlock_acquire(&child->p_lock);
 as_copy(as,&child->p_addrspace);
 spinlock_release(&child->p_lock);
 
 //pid_t is defined as something, I should figue out what that is...
 return child->pid;
}
#endif
