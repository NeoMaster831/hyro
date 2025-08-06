PUBLIC AVmxLaunchGuestBrdgIdPr
PUBLIC AVmxRestoreState
PUBLIC AVmxExitHandlerBrdg
PUBLIC AHyroVmcall

EXTERN VmxLaunchGuestIdPr:PROC
EXTERN VmxExitHandler:PROC
EXTERN VmxResume:PROC
EXTERN VmxReturnStackPointerForVmxoff:PROC
EXTERN VmxReturnInstructionPointerForVmxoff:PROC

.code _text

AVmxLaunchGuestBrdgIdPr proc
	
	; --- Save the host state ---
	pushfq

	; pushaq
	push rax
	push rcx
	push rdx
	push rbx
	push rbp
	push rsi
	push rdi
	push r8
	push r9
	push r10
	push r11
	push r12
	push r13
	push r14
	push r15
	; pushaq

	sub rsp, 100h
	; --- Save the host state ---

	mov rcx, rsp
	call VmxLaunchGuestIdPr
	
	int 3 ; We should never reach this point.
	; For debug purposes, we will break here if we do.

	jmp AVmxRestoreState
AVmxLaunchGuestBrdgIdPr endp

AVmxRestoreState proc
	; --- Restore the host state ---
	add rsp, 100h

	; popaq
	pop r15
	pop r14
	pop r13
	pop r12
	pop r11
	pop r10
	pop r9
	pop r8
	pop rdi
	pop rsi
	pop rbp
	pop rbx
	pop rdx
	pop rcx
	pop rax
	; popaq

	popfq
	; --- Restore the host state ---

	ret
AVmxRestoreState endp

AVmxExitHandlerBrdg PROC
    
    push 0  ; we might be in an unaligned stack state, so the memory before stack might cause 
            ; irql less or equal as it doesn't exist, so we just put some extra space avoid
            ; these kind of errors

    pushfq
    push r15
    push r14
    push r13
    push r12
    push r11
    push r10
    push r9
    push r8        
    push rdi
    push rsi
    push rbp
    push rbp	; rsp
    push rbx
    push rdx
    push rcx
    push rax	

    mov rcx, rsp
    sub	rsp, 020h
    call	VmxExitHandler
    add	rsp, 020h
    
    cmp	al, 1	
    je		AVmxoffHandler
    
    pop rax
    pop rcx
    pop rdx
    pop rbx
    pop rbp		; rsp
    pop rbp
    pop rsi
    pop rdi 
    pop r8
    pop r9
    pop r10
    pop r11
    pop r12
    pop r13
    pop r14
    pop r15
    popfq

    sub rsp, 0100h
    jmp VmxResume
    
AVmxExitHandlerBrdg ENDP

AVmxoffHandler PROC
    
    sub rsp, 020h
    call VmxReturnStackPointerForVmxoff
    add rsp, 020h
    
    mov [rsp+88h], rax
    
    sub rsp, 020h
    call VmxReturnInstructionPointerForVmxoff
    add rsp, 020h
    
    mov rdx, rsp
    mov rbx, [rsp+88h]
    mov rsp, rbx
    
    push rax
    mov rsp, rdx
    sub rbx,08h
    mov [rsp+88h], rbx
    
    pop rax
    pop rcx
    pop rdx
    pop rbx
    pop rbp		         ; rsp
    pop rbp
    pop rsi
    pop rdi 
    pop r8
    pop r9
    pop r10
    pop r11
    pop r12
    pop r13
    pop r14
    pop r15
    popfq
    pop		rsp

    ret

AVmxoffHandler ENDP

AHyroVmcall PROC
    
    ; The API Wrapper contains here with rax = 0, but here, we will not use it.
    ; Because we do all things in the VMX Root Mode, we don't have actual G-H
    ; transitions. So I will just remove the rax = 0 part.
	pushfq
	push r10
	push r11
	push r12
	mov r10, 80187c4d01ad09ccH
	mov r11, 1a2b99b4c7f9d191H
	mov r12, 913a2b99b4c7f9d1H
	vmcall
	pop r12
	pop r11
	pop r10
	popfq

	ret

AHyroVmcall ENDP

end