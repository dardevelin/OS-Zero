#include <stddef.h>
#include <kern/conf.h>
#include <kern/util.h>
#include <kern/mem.h>
#include <kern/proc.h>
#include <zero/param.h>
#include <kern/ia32/boot.h>
#include <kern/ia32/cpu.h>
#include <kern/ia32/vm.h>

struct proc proctab[NPROC];
struct thr  thrtab[NTHR];

long
procinit(long id)
{
    struct proc *proc = &proctab[id];
    void        *ptr;

    if (!id) {
        /* bootstrap */
        curproc = proc;
        proc->thr = &thrtab[0];
    }
    if (proc) {
        proc->state = PROCINIT;
        /* initialise page directory */
        bzero(proc, sizeof(struct proc));
        ptr = kmalloc(NPDE * sizeof(pde_t));
        if (ptr) {
            bzero(ptr, NPDE * sizeof(pde_t));
            proc->pdir = ptr;
        } else {
            kfree(proc);

            return -1;
        }
        /* initialise kernel-mode stack (wired) */
        ptr = kmalloc(KERNSTKSIZE);
        if (ptr) {
            bzero(ptr, KERNSTKSIZE);
            proc->kstk = ptr;
        } else {
            kfree(proc);
            kfree(proc->pdir);

            return -1;
        }
        /* initialise descriptor table */
        ptr = kmalloc(NDESCTAB * sizeof(desc_t));
        if (ptr) {
            bzero(ptr, NDESCTAB * sizeof(desc_t));
            proc->dtab = ptr;
        } else {
            kfree(proc);
            kfree(proc->pdir);
            kfree(proc->kstk);

            return -1;
        }
#if 0
        /* initialise VM structures */
        ptr = kmalloc(NVMHDRTAB * sizeof(struct vmpage));
        if (ptr) {
            bzero(ptr, NVMHDRTAB * sizeof(struct vmpage));
            proc->vmhdrtab = ptr;
        } else {
            kfree(proc);
            kfree(proc->pdir);
            kfree(proc->kstk);
            kfree(proc->dtab);

            return -1;
        }
#endif
        proc->state = PROCREADY;
    }

    return 0;
}

void *
procgetdesc(struct proc *proc, long id)
{
    void      *ret = NULL;
    long       lim = NDESCTAB - 1;
    long       val1;
    long       val2;
    uintptr_t *tab2;
    uintptr_t *tab3;

    if (id <= lim) {
        ret = (void *)(proc->dtab[id]);
    } else {
        tab2 = proc->dtab2;
        if (tab2) {
            val1 = id >> NDESCTABLOG2;
            val2 = id & (NDESCTAB - 1);
            tab3 = (uintptr_t *)(tab2[val1]);
            if (tab3) {
                ret = (void *)(tab3[val2]);
            }
        }
    }

    return ret;
}
