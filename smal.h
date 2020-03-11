/*
 * smal.h
 *
 *  Created on: Mar 6, 2020
 *      Author: msche
 */

#ifndef SMAL_H_
#define SMAL_H_
#include <msp430.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#define HZ      1000
#define QUANTUM 2
#define DEFAULT_STACK_SIZE  25 /* in 16 bit words */
#define NPROC   10
#define LIST_HDR_TLR_SIZE   2
#define STACK_MARKER    0xCEA1

#define enable_interrupts()   __bis_SR_register(GIE)
#define disable_interrupts()  __bic_SR_register(GIE)
#define q_empty(q, head)      ((q)[head].next == (head+1))

typedef enum {
    NOT_USED = 0,
    RUNNING,
    READY,
    SUSPENDED,
    SLEEPING,
} state_t;

typedef struct ProcessControlBlock {
    uint16_t *stack;
    uint16_t *stack_base;
    state_t   state;
    int8_t   priority;
} pcb_t;

typedef struct ListEntry {
    uint8_t next;
    int16_t val;
} le_t;

typedef uint8_t pid_t;

extern void ctxsw(uint16_t **old, uint16_t **new);
extern void _startx(uint16_t **new);
extern const uint8_t rl_head;
extern const uint8_t rl_tail;
extern le_t readylist[];
extern pcb_t pcb[];
extern pid_t currpid;

void sleep(uint16_t ms);
void ready(uint8_t pid);
pid_t create(void *func_ptr, int8_t priority);
bool l_pop(le_t list[], uint8_t head, uint8_t *ret);
void l_appendd(le_t list[], uint8_t head, uint8_t n, uint16_t d);
void l_appendv(le_t list[], uint8_t head, uint8_t n, int8_t v);
void l_append(le_t list[], uint8_t head, uint8_t n);
void initialize();
void start_smal();
void resched();
#endif /* SMAL_H_ */
