#include <types.h>
#include <kern/errno.h>
#include <kern/unistd.h>
#include <lib.h>
#include <uio.h>
#include <syscall.h>
#include <vnode.h>
#include <vfs.h>
#include <current.h>
#include <proc.h>
//#include "opt-A2.h"
//#include <unistd.h>
//#include <semaphore.h>
//#include <pthread.h> 

// #if OPT_A2

// #define MAX 1000

// vnode_t **file_desc[MAX];
// int index = 0;
// sem_t *sem_list[MAX];
// int readers[MAX];
// sem_t *db_list[MAX];

// int sys_open(const char *filename, int flags)
// {
//   mode_t mode = 0b111;
//   vnode_t *vnode;
//   KASSERT(vfs_open(filename,flags,mode,vnode));
//   file_desc[index] = vnode;
//   sem_init(&db_list[index],0,1);
//   sem_init(&sem_list[index],0,1);
//   readers[index] = 0;
//   return index++;
// }

// int sys_close(int fd) 
// {
//   vnode_t *vnode = file_desc[fd];
//   KASSERT(vfs_close(vnode));
//   sem_destroy(&sem_list[index]);
//   sem_destroy(&db_list[index]);
//   readers[index] = 0;
//   return 0;
// }
// #endif


/* handler for write() system call                  */
/*
 * n.b.
 * This implementation handles only writes to standard output 
 * and standard error, both of which go to the console.
 * Also, it does not provide any synchronization, so writes
 * are not atomic.
 *
 * You will need to improve this implementation
 */

int
sys_write(int fdesc,userptr_t ubuf,unsigned int nbytes,int *retval)
{
  struct iovec iov;
  struct uio u;
  int res;

  DEBUG(DB_SYSCALL,"Syscall: write(%d,%x,%d)\n",fdesc,(unsigned int)ubuf,nbytes);
  
  /* only stdout and stderr writes are currently implemented */
  if (!((fdesc==STDOUT_FILENO)||(fdesc==STDERR_FILENO))) {
    return EUNIMP;
  }
  KASSERT(curproc != NULL);
  KASSERT(curproc->console != NULL);
  KASSERT(curproc->p_addrspace != NULL);

  /* set up a uio structure to refer to the user program's buffer (ubuf) */
  iov.iov_ubase = ubuf;
  iov.iov_len = nbytes;
  u.uio_iov = &iov;
  u.uio_iovcnt = 1;
  u.uio_offset = 0;  /* not needed for the console */
  u.uio_resid = nbytes;
  u.uio_segflg = UIO_USERSPACE;
  u.uio_rw = UIO_WRITE;
  u.uio_space = curproc->p_addrspace;

  //res = VOP_WRITE(curproc->console,&u);
  #if OPT_A2
    // vnode_t *vnode = file_desc[fdesc];
    // sem_wait(&db_list[fdesc]);
    // res = VOP_WRITE(vnode,&u);
    // sem_post(&db_list[fdesc]);
    res = VOP_WRITE(curproc->console,&u);
  #else
    res = VOP_WRITE(curproc->console,&u);
  #endif
  if (res) {
    return res;
  }

  /* pass back the number of bytes actually written */
  *retval = nbytes - u.uio_resid;
  KASSERT(*retval >= 0);

  return 0;
}

// #if OPT_A2
// int sys_read(int fdesc,userptr_t ubuf,unsigned int nbytes,int* retvalue) {
//   struct iovec iov;
//   struct uio u;
//   int res;

//   DEBUG(DB_SYSCALL,"Syscall: write(%d,%x,%d)\n",fdesc,(unsigned int)ubuf,nbytes);
  
//   /* only stdout and stderr writes are currently implemented */
//   if (!((fdesc==STDOUT_FILENO)||(fdesc==STDERR_FILENO))) {
//     return EUNIMP;
//   }
//   KASSERT(curproc != NULL);
//   KASSERT(curproc->console != NULL);
//   KASSERT(curproc->p_addrspace != NULL);

//   /* set up a uio structure to refer to the user program's buffer (ubuf) */
//   iov.iov_ubase = ubuf;
//   iov.iov_len = nbytes;
//   u.uio_iov = &iov;
//   u.uio_iovcnt = 1;
//   u.uio_offset = 0;  /* not needed for the console */
//   u.uio_resid = nbytes;
//   u.uio_segflg = UIO_USERSPACE;
//   u.uio_rw = UIO_READ;
//   u.uio_space = curproc->p_addrspace;

//   vnode_t *vnode = file_desc[fdesc];
//   sem_wait(&sem_list[fdesc]);
//   readers[fdesc]++;
//   if (readers[fdesc] == 1)
//   {
//   sem_wait(&db_list[fdesc]);
//   }
//   sem_post(&sem_list[fdesc]);
//   res = VOP_READ(vnode,&u);
//   sem_wait(&sem_list[fdesc]);
//   readers[fdesc]--;
//   if (readers[fdesc] == 0)
//   {
//     sem_post(&db_list[fdesc]);
//   }
//   sem_post(&sem_list[fdesc]);

//   return 0;
// }
// #endif
