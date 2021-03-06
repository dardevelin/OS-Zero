#include <stddef.h>
#include <zero/cdecl.h>
#include <zero/param.h>
#include <zero/trix.h>
#include <zero/asm.h>
#include <kern/conf.h>
#include <kern/util.h>
#include <kern/obj.h>
#include <kern/proc/thr.h>
#include <kern/mem.h>
#include <kern/unit/x86/cpu.h>

static struct thrwait thrwaittab[NLVL0THR] ALIGNED(PAGESIZE);
static struct thrq    thrruntab[THRNCLASS * THRNPRIO];

/* save thread context */
void
thrsave(struct thr *thr)
{
    uint32_t pc;

    /* threads return to thryield() */
    m_getretadr(pc);
    m_tcbsave(&thr->m_tcb);
    thr->m_tcb.iret.eip = pc;

    return;
}

/* run thread */
void
thrjmp(struct thr *thr)
{
    curthr = thr;
    curproc = thr->proc;
    m_tcbjmp(&thr->m_tcb);

    /* NOTREACHED */
    return;
}

#if (ZEROSCHED)

/* adjust thread priority */
static __inline__ long
thradjprio(struct thr *thr)
{
    long class = thr->class;
    long prio = thr->prio;

    if (class != THRRT) {
        /* wrap around back to 0 at THRNPRIO / 2 */
        prio++;
        prio &= (THRNPRIO >> 1) - 1;
        prio = (THRNPRIO * class) + (THRNPRIO >> 1) + prio + thr->nice;
        prio = max(THRNPRIO * THRNCLASS - 1, prio);
        thr->prio = prio;
    }

    return prio;
}

/* add thread to end of queue */
void
thrqueue(struct thr *thr, struct thrq *thrq)
{
    mtxlk(&thrq->lk);
    thr->prev = thrq->tail;
    if (thr->prev) {
        thr->prev->next = thr;
    } else {
        thrq->head = thr;
    }
    thrq->tail = thr;
    mtxunlk(&thrq->lk);

    return;
}

/* add thread to wait queue */
long
thraddwait(struct thr *thr)
{
    struct thrwait *tab;
    struct thrwait *ptr = NULL;
    long            wchan = thr->wchan;
    long            ret = -1;
    long            fail = 0;    
    uint64_t        key0;
    uint64_t        key1;
    uint64_t        key2;
    uint64_t        key3;
    void           *pstk[THRNKEY] = { NULL, NULL, NULL, NULL };

    key0 = thrwaitkey0(wchan);
    key1 = thrwaitkey1(wchan);
    key2 = thrwaitkey2(wchan);
    key3 = thrwaitkey3(wchan);
    tab = &thrwaittab[key0];
    mtxlk(&tab->lk);
    ptr = tab->ptr;
    if (!ptr) {
        ptr = kmalloc(NLVL0THR * sizeof(struct thrwait));
        if (ptr) {
            kbzero(ptr, NLVL0THR * sizeof(struct thrwait));
        }
        pstk[0] = ptr;
    }
    if (ptr) {
        ptr = ptr[key0].ptr;
        if (!ptr) {
            ptr = kmalloc(NLVL1THR * sizeof(struct thrwait));
            if (ptr) {
                kbzero(ptr, NLVL1THR * sizeof(struct thrwait));
            }
            pstk[1] = ptr;
        }
    } else {
        fail = 1;
    }
    if (ptr) {
        ptr = ptr[key1].ptr;
        if (!ptr) {
            ptr = kmalloc(NLVL2THR * sizeof(struct thrwait));
            if (ptr) {
                kbzero(ptr, NLVL2THR * sizeof(struct thrwait));
            }
            pstk[2] = ptr;
        }
    } else {
        fail = 1;
    }
    if (ptr) {
        ptr = ptr[key2].ptr;
        if (!ptr) {
            ptr = kmalloc(NLVL3THR * sizeof(struct thrwait));
            if (ptr) {
                kbzero(ptr, NLVL3THR * sizeof(struct thrwait));
            }
            pstk[3] = ptr;
        }
    } else {
        fail = 1;
    }
    if (!fail) {
        if (pstk[0]) {
            tab->nref++;
            tab->ptr = pstk[0];
        }
        tab = tab->ptr;
        if (pstk[1]) {
            tab[key0].nref++;
            tab[key0].ptr = pstk[1];
        }
        tab = tab[key0].ptr;
        if (pstk[2]) {
            tab[key1].nref++;
            tab[key1].ptr = pstk[2];
        }
        tab = tab[key1].ptr;
        if (pstk[3]) {
            tab[key2].nref++;
            tab[key2].ptr = pstk[3];
        }
        tab = tab[key2].ptr;
        tab = &tab[key3];
        tab->nref++;
//        thr->prev = NULL;
        thr->next = tab->ptr;
#if 0
        if (thr->next) {
            thr->next->prev = thr;
        }
#endif
        tab->ptr = thr;
        ret ^= ret;
    }
    mtxunlk(&tab->lk);
    
    return ret;
}

/* move a thread from wait queue to ready queue */
void
thrwakeup(uintptr_t wchan)
{
    struct thrwait *tab;
    struct thrwait *ptr = NULL;
    struct thr     *thr1;
    struct thr     *thr2;
    long            key0 = thrwaitkey0(wchan);
    long            key1 = thrwaitkey1(wchan);
    long            key2 = thrwaitkey2(wchan);
    long            key3 = thrwaitkey3(wchan);
//    long            n;
    struct thrwait *pstk[4] = { NULL };

    tab = &thrwaittab[key0];
    mtxlk(&tab->lk);
    ptr = tab;
    if (ptr) {
        pstk[0] = ptr;
        ptr = ((void **)ptr->ptr)[key0];
        if (ptr) {
            pstk[1] = ptr;
            ptr = ((void **)ptr->ptr)[key1];
            if (ptr) {
                pstk[2] = ptr;
                ptr = ((void **)ptr->ptr)[key2];
                if (ptr) {
                    pstk[3] = ptr;
                    ptr = ((void **)ptr->ptr)[key3];
                }
            }
        }
        thr1 = ptr->ptr;
        if (thr1) {
            thr2 = thr1->next;
            thrqueue(thr1, &thrruntab[thr1->prio]);
            ptr->ptr = thr2;
        }
#if 0
        while (thr1) {
            thr2 = thr1->next;
            thrqueue(thr1, &thrruntab[thr1->prio]);
            thr1 = thr2;
        }
#endif
        /* TODO: free tables if possible */
        ptr = pstk[0];
        if (ptr) {
            if (!--ptr->nref) {
                kfree(ptr->ptr);
                ptr->ptr = NULL;
            }
            ptr = pstk[1];
            if (ptr) {
                if (!--ptr->nref) {
                    kfree(ptr->ptr);
                    ptr->ptr = NULL;
                }
                ptr = pstk[2];
                if (ptr) {
                    if (!--ptr->nref) {
                        kfree(ptr->ptr);
                        ptr->ptr = NULL;
                    }
                    ptr = pstk[3];
                    if (ptr) {
                        if (!--ptr->nref) {
                            kfree(ptr->ptr);
                            ptr->ptr = NULL;
                        }
                    }
                }
            }
        }
    }
    mtxunlk(&tab->lk);
}

/* switch threads */
void
thryield(void)
{
    struct thr  *thr = NULL;
    struct thrq *thrq;
    long         prio;
    long         state = curthr->state;

    thrsave(curthr);
    prio = thradjprio(curthr);
    if (state == THRREADY) {
        thrq = &thrruntab[prio];
        thrqueue(curthr, thrq);
    } else if (state == THRWAIT) {
        thraddwait(curthr);
    }
    while (!thr) {
        for (prio = 0 ; prio < THRNCLASS * THRNPRIO ; prio++) {
            thrq = &thrruntab[prio];
            mtxlk(&thrq->lk);
            thr = thrq->head;
            if (thr) {
                if (thr->next) {
                    thr->next->prev = thr;
                } else {
                    thrq->tail = NULL;
                }
                thrq->head = thr->next;
                mtxunlk(&thrq->lk);
                if (thr != curthr) {
                    thrjmp(thr);
                } else {

                    return;
                }
            } else {
                mtxunlk(&thrq->lk);
            }
        }
        if (!thr) {
            m_waitint();
        }
    }

    return;
}

#endif /* ZEROSCHED */

