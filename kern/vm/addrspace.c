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

#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <addrspace.h>
#include <vm.h>
#include <proc.h>
#include <elf.h>
#include <spl.h>
#include <cpu.h>
#include <spinlock.h>
#include <proc.h>
#include <current.h>
#include <mips/tlb.h>
#include <bitmap.h>
/*
 * Note! If OPT_DUMBVM is set, as is the case until you start the VM
 * assignment, this file is not compiled or linked or in any way
 * used. The cheesy hack versions in dumbvm.c are used instead.
 */

struct addrspace *
as_create(void)
{
	struct addrspace *as;

	as = kmalloc(sizeof(struct addrspace));
	if (as == NULL) {
		return NULL;
	}

	/*
	 * Initialize as needed.
	 */
	as->pageTable = NULL;
	as->regionInfo = NULL;
	as->heap_vbase = 0;
	as->heap_vbound = 0;
	// as->heap_page_used = 0;
	return as;
}



void
as_destroy(struct addrspace *as)
{
	/*
	 * Clean up as needed.
	 */
	// (void)as;
	if(as == NULL){
		return;
	}
	struct pageTableNode * ptTmp = as->pageTable;
	struct pageTableNode * ptTmp2 = NULL;


	while(ptTmp != NULL){
		ptTmp2 = ptTmp;
		ptTmp = ptTmp->next;
		if(ptTmp2->pt_inDisk){
			KASSERT(bitmap_isset(vm_bitmap, ptTmp2->pt_diskOffset / PAGE_SIZE) == true);
			bitmap_unmark(vm_bitmap, ptTmp2->pt_diskOffset / PAGE_SIZE);
		}else{
			free_kpages(PADDR_TO_KVADDR(ptTmp2->pt_pas));
		}
		kfree(ptTmp2);
	}

	struct regionInfoNode * riTmp = as->regionInfo;
	struct regionInfoNode * riTmp2 = NULL;
	while(riTmp != NULL){
		riTmp2 = riTmp;
		riTmp = riTmp->next;
		kfree(riTmp2);
	}

	kfree(as);
}

void
as_activate(void)
{
	int i, spl;
	struct addrspace *as;

	as = proc_getas();
	if (as == NULL) {
		return;
	}

	/* Disable interrupts on this CPU while frobbing the TLB. */
	spl = splhigh();

	for (i=0; i<NUM_TLB; i++) {
		// kprintf("%d",i);
		tlb_write(TLBHI_INVALID(i), TLBLO_INVALID(), i);
	}

	splx(spl);
}

void
as_deactivate(void)
{
	/*
	 * Write this. For many designs it won't need to actually do
	 * anything. See proc.c for an explanation of why it (might)
	 * be needed.
	 */
}

/*
 * Set up a segment at virtual address VADDR of size MEMSIZE. The
 * segment in memory extends from VADDR up to (but not including)
 * VADDR+MEMSIZE.
 *
 * The READABLE, WRITEABLE, and EXECUTABLE flags are set if read,
 * write, or execute permission should be set on the segment. At the
 * moment, these are ignored. When you write the VM system, you may
 * want to implement them.
 */
int
as_define_region(struct addrspace *as, vaddr_t vaddr, size_t memsize,
		 int readable, int writeable, int executable)
{
	size_t npages;

	/* Align the region. First, the base... */
	memsize += vaddr & ~(vaddr_t)PAGE_FRAME;
	vaddr &= PAGE_FRAME;

	/* ...and now the length. */
	memsize = (memsize + PAGE_SIZE - 1) & PAGE_FRAME;

	npages = memsize / PAGE_SIZE;

	int permission = readable | writeable | executable;

	struct regionInfoNode * tmp = (struct regionInfoNode*)kmalloc(sizeof(struct regionInfoNode));
	if(tmp == NULL){
        return ENOMEM;
    }
	tmp->as_vbase = vaddr;
	tmp->as_npages = npages;
	tmp->as_permission = permission;// code & data = readonly
	// tmp->as_tmp_permission = permission;

	tmp->next = as->regionInfo;
	as->regionInfo = tmp;
	// tmp->next = NULL;
	// if(as->regionInfo == NULL){
	// 	as->regionInfo = tmp;
	// }else{
	// 	tmp->next = as->regionInfo->next;
	// 	as->regionInfo->next = tmp;
	// }
	if(as->heap_vbase < tmp->as_vbase + tmp->as_npages * PAGE_SIZE){
		as->heap_vbase = tmp->as_vbase + tmp->as_npages * PAGE_SIZE;
	}
	return 0;

}

int
as_prepare_load(struct addrspace *as)
{
	/*
	 * Write this.
	 */
	// change all regions' permission to read & write
	// struct regionInfoNode * tmp = as->regionInfo;
	// while(tmp != NULL){
	// 	tmp->as_permission = PF_R | PF_W;
	// 	tmp = tmp->next;
	// }
	// as->as_stackpbase = alloc_kpages(VM_STACKPAGES);
	// if (as->as_stackpbase == 0) {
	// 	return ENOMEM;
	// }
	(void)as;
	return 0;
}

int
as_complete_load(struct addrspace *as)
{
	/*
	 * Write this.
	 */
	(void)as;
	// struct regionInfoNode * tmp = as->regionInfo;
 // 	while(tmp != NULL){
 // 		tmp->as_permission = tmp->as_tmp_permission;
 // 		tmp = tmp->next;
 // 	}
	return 0;
}

int
as_define_stack(struct addrspace *as, vaddr_t *stackptr)
{
	/*
	 * Write this.
	 */
	(void)as;
	*stackptr = USERSTACK;
	return 0;
}


int
as_copy(struct addrspace *old, struct addrspace **ret)
{
	struct addrspace *newas;

	newas = as_create();
	if (newas==NULL) {
		return ENOMEM;
	}

	/*
	 * Write this.
	 */
	vaddr_t vaddr_tmp;
	newas->heap_vbase = old->heap_vbase;
	newas->heap_vbound = old->heap_vbound;

	//pageTable
	newas->pageTable = (struct pageTableNode*)kmalloc(sizeof(struct pageTableNode));
	if(newas->pageTable == NULL){
		return ENOMEM;
	}

	struct pageTableNode *oldPTtmp = old->pageTable;
	//pageTable Head
	if(oldPTtmp != NULL){
		newas->pageTable->pt_vas = oldPTtmp->pt_vas;
		newas->pageTable->pt_isDirty = true;
		newas->pageTable->pt_inDisk = false;
		newas->pageTable->pt_diskOffset = 0;
		newas->pageTable->next = NULL;
		vaddr_tmp = user_alloc_onepage();//alloc_kpages(1);
		if(vaddr_tmp == 0){
			as_destroy(newas);
			return ENOMEM;
		}
		newas->pageTable->pt_pas = vaddr_tmp - MIPS_KSEG0;
		bzero((void *)PADDR_TO_KVADDR(newas->pageTable->pt_pas), 1 * PAGE_SIZE);
		if(oldPTtmp->pt_inDisk){
			if(block_read((void *)PADDR_TO_KVADDR(newas->pageTable->pt_pas), oldPTtmp->pt_diskOffset)){
				kprintf("block_read error in as_copy\n");
			}
		}else{
			memmove((void *)PADDR_TO_KVADDR(newas->pageTable->pt_pas),
				(const void *)PADDR_TO_KVADDR(oldPTtmp->pt_pas),
				1*PAGE_SIZE);
		}
	}
	//copy pageTable
	struct pageTableNode *PTtmp = newas->pageTable;
	struct pageTableNode *PTtmp2;
	while(oldPTtmp != NULL){
		//PTtmp2 init
		PTtmp2 = (struct pageTableNode*)kmalloc(sizeof(struct pageTableNode));
		if(PTtmp2 == NULL){
			as_destroy(newas);
			return ENOMEM;
		}
		PTtmp2->pt_vas = oldPTtmp->pt_vas;
		PTtmp2->pt_isDirty = true;
		PTtmp2->pt_inDisk = false;
		PTtmp2->pt_diskOffset = 0;
		PTtmp2->next = NULL;
		//memory
		vaddr_tmp = user_alloc_onepage();//alloc_kpages(1);
		if(vaddr_tmp == 0){
			as_destroy(newas);
			return ENOMEM;
		}
		PTtmp2->pt_pas = vaddr_tmp - MIPS_KSEG0;
		bzero((void *)PADDR_TO_KVADDR(PTtmp2->pt_pas), 1 * PAGE_SIZE);
		if(oldPTtmp->pt_inDisk){
			if(block_read((void *)PADDR_TO_KVADDR(PTtmp2->pt_pas), oldPTtmp->pt_diskOffset)){
				kprintf("block_read error in as_copy\n");
			}
		}else{
			memmove((void *)PADDR_TO_KVADDR(PTtmp2->pt_pas),
				(const void *)PADDR_TO_KVADDR(oldPTtmp->pt_pas),
				1*PAGE_SIZE);
		}

		//link
		PTtmp->next = PTtmp2;
		PTtmp = PTtmp->next;
		oldPTtmp = oldPTtmp->next;
	}

	//regionInfo
	newas->regionInfo = (struct regionInfoNode*)kmalloc(sizeof(struct regionInfoNode));
	if(newas->regionInfo == NULL){
		as_destroy(newas);
		return ENOMEM;
	}
	struct regionInfoNode *oldRItmp = old->regionInfo;
	//regionInfo Head
	if(oldRItmp != NULL){
		newas->regionInfo->as_vbase = oldRItmp->as_vbase;
		newas->regionInfo->as_npages = oldRItmp->as_npages;
		newas->regionInfo->as_permission = oldRItmp->as_permission;
		newas->regionInfo->next = NULL;
	}
	//copy regionInfo
	struct regionInfoNode *RItmp = newas->regionInfo;
	struct regionInfoNode *RItmp2;
	while(oldRItmp != NULL){
		//RItmp2 init
		RItmp2 = (struct regionInfoNode*)kmalloc(sizeof(struct regionInfoNode));
		if(RItmp2 == NULL){
			as_destroy(newas);
			return ENOMEM;
		}
		RItmp2->as_vbase = oldRItmp->as_vbase;
		RItmp2->as_npages = oldRItmp->as_npages;
		RItmp2->as_permission = oldRItmp->as_permission;
		RItmp2->next = NULL;
		//link
		RItmp->next = RItmp2;
		RItmp = RItmp->next;
		oldRItmp = oldRItmp->next;
	}

	*ret = newas;
	return 0;
}
