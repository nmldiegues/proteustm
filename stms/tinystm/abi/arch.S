	.text

	.align 4
	.globl	_ITM_beginTransaction
	.type	_ITM_beginTransaction, @function

_ITM_beginTransaction:
	.cfi_startproc
/* Paramters (in order) is in rdi, rsi, rdx, rcx, r8, r9 */
/* Temporary registers is r10, r11 (not saved) */
/* To be saved are rbx, rsp, rbp, r12, r13, r14, r15 */
	leaq	8(%rsp), %rax    /* Save stack pointer */
	subq	$56, %rsp
	.cfi_def_cfa_offset 64
	movq	%rax, (%rsp)
	movq	%rbx, 8(%rsp)
	movq	%rbp, 16(%rsp)
	movq	%r12, 24(%rsp)
	movq	%r13, 32(%rsp)
	movq	%r14, 40(%rsp)
	movq	%r15, 48(%rsp)
	movq	%rsp, %rsi
	call	GTM_begin_transaction
	addq	$56, %rsp
	.cfi_def_cfa_offset 8
	ret
	.cfi_endproc
	.size	_ITM_beginTransaction, .-_ITM_beginTransaction

	.align 4
	.globl	_ITM_siglongjmp
	.type	_ITM_siglongjmp, @function
	.hidden	_ITM_siglongjmp

_ITM_siglongjmp:
	.cfi_startproc
	movq	(%rsi), %rcx
	movq	8(%rsi), %rbx
	movq	16(%rsi), %rbp
	movq	24(%rsi), %r12
	movq	32(%rsi), %r13
	movq	40(%rsi), %r14
	movq	48(%rsi), %r15
	movl	%edi, %eax
	.cfi_def_cfa %rsi, 0
	.cfi_offset %rip, 56
	.cfi_register %rsp, %rcx
	movq	%rcx, %rsp
	jmp	*56(%rsi)
	.cfi_endproc
	.size	_ITM_siglongjmp, .-_ITM_siglongjmp

.section .note.GNU-stack, "", @progbits
