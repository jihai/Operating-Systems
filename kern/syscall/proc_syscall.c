#include <types.h>
#include <limits.h>
#include <current.h>
#include <mips/trapframe.h>
#include <proc.h>
#include <addrspace.h>
#include <kern/errno.h>
#include <proc_syscall.h>
#include <vnode.h>
#include <copyinout.h>
#include <syscall.h>
#include <kern/wait.h>


int
sys_getpid(pid_t * retval){
    *retval = curproc->p_PID;
    return 0;
}

int sys_fork(struct trapframe * tf, int * retval){
    //copy parent's tf to child's new trapframe
    struct trapframe * newtf = NULL;
    newtf = (struct trapframe *)kmalloc(sizeof(struct trapframe));
    if(newtf == NULL){
        return ENOMEM;
    }
    memcpy(newtf, tf, sizeof(struct trapframe));

    //copy parent's as to child's new addrspace
    struct addrspace * newas = NULL;
    int result = 0;
    result = as_copy(curproc->p_addrspace, &newas);
    if(newas == NULL){
        kfree(newtf);
        return ENOMEM;
    }

    //create new proc, set child's PPID to parent's PID
    struct proc * newproc = NULL;
    newproc = proc_create_runprogram("child");
    if(newproc == NULL){
        kfree(newtf);
        as_destroy(newas);//now same as kfree(newas)
        return ENOMEM;
    }
    newproc->p_PPID = curproc->p_PID;
    /* copy filetable from proc to newproc
	file handle is not null, increase reference num by 1 */
    for(int fd=0;fd<OPEN_MAX;fd++)
	{
        newproc->fileTable[fd] = curproc->fileTable[fd];
        if(newproc->fileTable[fd] != NULL){
			newproc->fileTable[fd]->refcount++;
		}
	}
    // newproc->p_addrspace = newas;

    // thread_fork do the remaining work
    result = thread_fork("test_thread_fork", newproc, enter_forked_process, newtf, (unsigned long) newas);//data1, data2
    if(result) {
        return result;
    }
    *retval = newproc->p_PID;//could panic if just return p_PID
    // spinlock_acquire(&curproc->p_lock);
	// if (curproc->p_cwd != NULL) {
	// 	VOP_INCREF(curproc->p_cwd);
	// 	newproc->p_cwd = curproc->p_cwd;
	// }
	// spinlock_release(&curproc->p_lock);
    curproc->p_numthreads++;
    return 0;
}

// void
// entrypoint(void *data1, unsigned long data2){
//
//     struct trapframe tf;
//     // tf = (struct trapframe *)kmalloc(sizeof(struct trapframe));
//     struct trapframe* newtf = (struct trapframe*) data1;
//     struct addrspace* newas = (struct addrspace*) data2;
//     tf.tf_v0 = 0;//retval
//     tf.tf_a3 = 0;//return 0 = success
//     tf.tf_epc += 4;
//     /*
//     *Upon syscall return, the PC stored in the trapframe (tf_epc) must
//     *be incremented by 4 bytes, which is the length of one instruction.
//     *Otherwise the return code restart at the same point.
//     */
//     memcpy(&tf, newtf, sizeof(struct trapframe));
//     kfree(newtf);
//     newtf = NULL;
//     // proc_setas(newas);
//     curproc->p_addrspace = newas;
//     as_activate();
//     mips_usermode(&tf);
// }

int sys_waitpid(pid_t pid, userptr_t status, int options, pid_t *retval) {
	if(options != 0){
		return EINVAL;
	}
    if(pid < PID_MIN || pid >= PID_MAX){
        return ESRCH;
    }
	int exitstatus;
	int result;
	struct proc * p = procTable[pid];

	if(p == NULL){
		return ESRCH;
	}
	if(p->p_PPID != curproc->p_PID){
		// Current process can't wait on itself
		return ECHILD;
	}

	lock_acquire(p->p_lk);
		while(!p->p_exit) {
			cv_wait(p->p_cv, p->p_lk);
		}
	lock_release(p->p_lk);

	exitstatus = p->p_exitcode;
	result = copyout((void *)&exitstatus, status, sizeof(int));
	if (result) {
		return result;
	}

	*retval = pid;
	return 0;
}

void sys__exit(int exitcode) {
    struct proc * p = curproc;
    // as_deactivate();
    // as_destroy(proc_getas());
    KASSERT(procTable[p->p_PID] != NULL);
    proc_remthread(curthread);
    p->p_exit = true;
    p->p_exitcode = _MKWAIT_EXIT(exitcode);

    lock_acquire(p->p_lk);
    cv_broadcast(p->p_cv, p->p_lk);
    lock_release(p->p_lk);
    proc_destroy(p);
    thread_exit();
    panic("sys__exit--thread_exit failed");
}
