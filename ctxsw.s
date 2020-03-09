 		;.cdecls C, list, "msp430.h"
		.text
		.global ctxsw
		.global _startx

;switch to a new process, without saving current context
_startx:
	MOV		0(R12), SP
	POP		R15
	POP		R14
	POP		R13
	POP		R12
	POP		SR
	RETA

;context switch
;input 1: pointer to the stack of the old process (R12)
;input 2: pointer to the stack of the new process (R13)
ctxsw:
	;save the old processes context
	PUSH	SR
	PUSH	R12
	PUSH	R13
	PUSH	R14
	PUSH	R15
	MOV		SP, 0(R12)

	;switch to new function
	MOV		0(R13), SP ;put the new stack pointer up
	POP		R15
	POP		R14
	POP		R13
	POP		R12
	POP		SR
	RETA
