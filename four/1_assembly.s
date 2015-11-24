# void thread_switch(thread *old, thread *new);
.globl thread_switch
thread_switch:
	# Push all callee-save registers onto current stack
	pushq %rbx
	pushq %rbp
	pushq %r12
	pushq %r13
	pushq %r14
	pushq %r15
	
	# Save current stack pointer in old thread table entry
	movq %rsp, (%rdi)
	
	# Load the stack pointer from the new thread to curr stack ptr
	movq (%rsi), %rsp

	# Pop all callee_save registers from the new stack
	popq %r15
	popq %r14
	popq %r13
	popq %r12
	popq %rbp
	popq %rbx

	# Return
	ret

# void thread_start(thread *old, thread *new);
.globl thread_start
thread_start:
	# Push all callee-save registers onto the current stack
	pushq %rbx
	pushq %rbp
	pushq %r12
	pushq %r13
	pushq %r14
	pushq %r15

	# Save the current stack pointer in old thread table entry
	movq %rsp, (%rdi)

	# Load the stack pointer from new thread table entry into %rsp.
	movq (%rsi), %rsp

	# Jump to C wrapper function
	jmp thread_wrap 
