/*
 * smal.c
 * My small, MSP430 based micro OS.
 */
#include <smal.h>

le_t queue[NPROC + LIST_HDR_TLR_SIZE + LIST_HDR_TLR_SIZE];
pcb_t pcb[NPROC];

const uint8_t rl_head = NPROC;
const uint8_t rl_tail = NPROC+1;
const uint8_t sq_head = NPROC+2;
const uint8_t sq_tail = NPROC+3;

pid_t currpid;
uint16_t    preempt;

void null();
void hwinit();

static pid_t next_pid() {
    pid_t i = 0;

    do {
        i = (i+1) % NPROC;
    } while (pcb[i].state != NOT_USED);

    return i;
}

/* append a process to the readylist based on priority */
void ready(uint8_t pid) {
    pcb_t *p = &pcb[pid];
    p->state = READY;
    l_appendv(queue, rl_head, pid, p->priority);
}

pid_t create(void *func_ptr, int8_t priority) {
    uint16_t *stptr;
    pid_t new_pid = next_pid();
    pcb_t *p = &pcb[new_pid];

    /* allocate a new stack */
    stptr = (uint16_t*)malloc(sizeof(uint16_t)*DEFAULT_STACK_SIZE);
    p->stack_base = stptr;
    stptr += DEFAULT_STACK_SIZE-1; /* go to the end of the stack */
    *stptr-- = STACK_MARKER;
    *stptr-- = 0x0000;
    *stptr-- = (uint16_t)func_ptr;
    *stptr-- = 0x0008; /* SR, with interrupts enabled */
    *stptr-- = 0; /* R12 */
    *stptr-- = 0; /* R13 */
    *stptr-- = 0; /* R14 */
    *stptr = 0; /* R15 */
    p->stack = stptr;
    p->state = SUSPENDED;
    p->priority = priority;
    return new_pid;
}

inline int16_t l_peek(le_t list[], uint8_t head) {
    return list[list[head].next].val;
}

/* delta list append, sort by the key, d, but positions are differences
 * to the neighbors */
void l_appendd(le_t list[], uint8_t head, uint8_t n, uint16_t d) {
    uint8_t next, prev;

    /* find placement in list */
    prev = head;
    next = list[head].next;
    while (next != head+1 && d >= list[next].val) {
        d -= list[next].val;
        prev = next;
        next = list[next].next;
    }

    /* link in new entry */
    list[n].next = next;
    list[n].val = d;
    list[prev].next = n;

    /* update next entry, if not the tail */
    if (next != head+1) {
        list[next].val -= d;
    }
}

/* list append, sorted by key */
void l_appendv(le_t list[], uint8_t head, uint8_t n, int8_t v) {
    le_t *ent;
    ent = &list[head];

    while(list[ent->next].val >= v) {
        ent = &list[ent->next];
    }

    list[n].next = ent->next;
    ent->next = n;
    list[n].val = v;
}

void l_append(le_t list[], uint8_t head, uint8_t n) {
    le_t *ent;
    ent = &list[head];

    while (ent->next != head+1) {
        ent = &list[ent->next];
    }

    ent->next = n;
    list[n].next = head+1;
}

bool l_pop(le_t list[], uint8_t head, uint8_t *ret) {
    le_t *ent = &list[head];

    if (ent->next == head+1) {
        return 1;
    } else {
        *ret = ent->next;
        ent->next = list[ent->next].next;
        return 0;
    }
}

void initialize() {
    int i;
    pcb_t *p;
    uint16_t *stptr;

    hwinit();

    for (i = 0; i < NPROC; i++) {
        p = &pcb[i];
        p->state = NOT_USED;
    }

    /* initialize the readylist */
    queue[rl_head].next = rl_tail;
    queue[rl_head].val = 0x7fff;
    queue[rl_tail].next = rl_head;
    queue[rl_tail].val = 0x8000;

    /* initialize the sleep queue */
    queue[sq_head].next = sq_tail;
    queue[sq_head].val = 0x0000;
    queue[sq_tail].next = sq_head;
    queue[sq_tail].val = 0x7fff;

    preempt = 0;

    /* create the null process but only allocate, dont initialize the stack */
    //create(null, 0);
    p = &pcb[0];
    stptr = (uint16_t*)malloc(sizeof(uint16_t)*DEFAULT_STACK_SIZE);
    p->stack_base = stptr;
    stptr += DEFAULT_STACK_SIZE-1; /* go to the end of the stack */
    *stptr-- = STACK_MARKER;
    *stptr-- = 0x0000;
    *stptr-- = (uint16_t)(void *)null;
    *stptr-- = 0x0008; /* SR, with interrupts enabled */
    *stptr-- = 0; /* R12 */
    *stptr-- = 0; /* R13 */
    *stptr-- = 0; /* R14 */
    *stptr = 0; /* R15 */
    p->stack = stptr;
    p->state = SUSPENDED;
    p->priority = 0;
    ready(0);
}

/* get the highest priority process that is ready to run,
 * enable interrupts, then hard context switch to it.
 * the current context is abandoned. */
void start_smal() {
    pid_t p;
    l_pop(queue, rl_head, &p);
    pcb[p].state = RUNNING;
    currpid = p;
    preempt = QUANTUM;
    RTCCTL |= RTCSS__VLOCLK /*| RTCSR_L*/ | RTCIE;
    enable_interrupts();
    _startx(&pcb[p].stack);
}

void resched() {
    pcb_t *pold, *pnew;

    pold = &pcb[currpid];
    if (pold->state == RUNNING) {
        /* process was preempted, but still running */
        /* check if its still the highest priority */
        if (pold->priority > l_peek(queue, rl_head)) {
            /* it is, keep it running */
            preempt = QUANTUM;
            return;
        }

        /* it's not, put old process on readylist */
        ready(currpid);
    }

    /* get the new process */
    l_pop(queue, rl_head, &currpid);
    pnew = &pcb[currpid];
    pnew->state = RUNNING;
    preempt = QUANTUM;
    ctxsw(&pold->stack, &pnew->stack);
    return;
}

void sleep(uint16_t ms) {

    /* put the process on the sleep queue */
    disable_interrupts();
    l_appendd(queue, sq_head, currpid, ms);
    pcb[currpid].state = SLEEPING;
    resched();
    enable_interrupts();
}

void null() {
    while(1)
        ;
}

#pragma vector=RTC_VECTOR
__interrupt void rtc_irq(void) {
    uint8_t pid;
    unsigned int src = RTCIV; //clear the interrupt

    /* decrement sleep q */
    if (!q_empty(queue, sq_head)) {
        if (--queue[queue[sq_head].next].val <= 0) {
            l_pop(queue, sq_head, &pid);
            ready(pid);
        }
    }

    if (--preempt <= 0) {
        resched();
    }
}

void hwinit() {
    RTCCTL = RTCSR_L;
    RTCMOD = (1000/HZ) - 1;
    RTCCTL |= /*RTCSS__VLOCLK | RTCSR_L | RTCIE |*/ RTCPS__10;
}
