PUBLIC ACallHyro

.code _text

ACallHyro PROC
	
	mov rax, 0 ; Initially false
	pushfq
	push r10
	push r11
	push r12
	mov r10, 80187c4d01ad09ccH ; HYRO_SIGNATURE_LOW
	mov r11, 1a2b99b4c7f9d191H ; HYRO_SIGNATURE_MEDIUM
	mov r12, 913a2b99b4c7f9d1H ; HYRO_SIGNATURE_HIGH
	vmcall
	pop r12
	pop r11
	pop r10
	popfq

	ret

ACallHyro ENDP

END