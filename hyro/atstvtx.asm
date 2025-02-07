PUBLIC ATstHyroVmcall

.code _text

ATstHyroVmcall PROC

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

ATstHyroVmcall ENDP

END