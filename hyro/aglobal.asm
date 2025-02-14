PUBLIC AGetCs
PUBLIC AGetDs
PUBLIC ASetDs
PUBLIC AGetEs
PUBLIC ASetEs
PUBLIC AGetSs
PUBLIC ASetSs
PUBLIC AGetFs
PUBLIC ASetFs
PUBLIC AGetGs
PUBLIC AGetLdtr
PUBLIC AGetTr
PUBLIC AGetGdtBase
PUBLIC AGetIdtBase
PUBLIC AGetGdtLimit
PUBLIC AGetIdtLimit
PUBLIC AGetAccessRights
PUBLIC AGetRflags
PUBLIC AReloadGdtr
PUBLIC AReloadIdtr

PUBLIC AInvept
PUBLIC AInvVpid

.code _text

AGetGdtBase PROC

    LOCAL   gdtr[10]:BYTE
    sgdt    gdtr
    mov     rax, QWORD PTR gdtr[2]
    ret

AGetGdtBase ENDP

AGetCs PROC

    mov     rax, cs
    ret

AGetCs ENDP

AGetDs PROC

    mov     rax, ds
    ret

AGetDs ENDP

ASetDs PROC

    mov     rax, rcx
    mov     ds, rax 
    ret

ASetDs ENDP

AGetEs PROC

    mov     rax, es
    ret

AGetEs ENDP

ASetEs PROC

    mov     rax, rcx
    mov     es, rax 
    ret

ASetEs ENDP

AGetSs PROC

    mov     rax, ss
    ret

AGetSs ENDP

ASetSs PROC

    mov     rax, rcx
    mov     ss, rax 
    ret

ASetSs ENDP

AGetFs PROC

    mov     rax, fs
    ret

AGetFs ENDP

ASetFs PROC

    mov     rax, rcx
    mov     fs, rax 
    ret

ASetFs ENDP

AGetGs PROC

    mov     rax, gs
    ret

AGetGs ENDP

AGetLdtr PROC

    sldt    rax
    ret

AGetLdtr ENDP

AGetTr PROC

    str     rax
    ret

AGetTr ENDP

AGetIdtBase PROC

    LOCAL   idtr[10]:BYTE
    
    sidt    idtr
    mov     rax, QWORD PTR idtr[2]
    ret

AGetIdtBase ENDP

AGetGdtLimit PROC

    LOCAL    gdtr[10]:BYTE
    
    sgdt    gdtr
    mov     ax, WORD PTR gdtr[0]
    ret

AGetGdtLimit ENDP

AGetIdtLimit PROC

    LOCAL    idtr[10]:BYTE
    
    sidt    idtr
    mov     ax, WORD PTR idtr[0]
    ret

AGetIdtLimit ENDP

AGetAccessRights PROC
    lar     rax, rcx
    jz      no_error
    xor     rax, rax
no_error:
    ret
AGetAccessRights ENDP

AGetRflags PROC
    
    pushfq
    pop		rax
    ret
    
AGetRflags ENDP

AReloadGdtr PROC

    push	rcx
    shl		rdx, 48
    push	rdx
    lgdt	fword ptr [rsp+6]
    pop		rax
    pop		rax
    ret
    
AReloadGdtr ENDP

AReloadIdtr PROC
    
    push	rcx
    shl		rdx, 48
    push	rdx
    lidt	fword ptr [rsp+6]
    pop		rax
    pop		rax
    ret
    
AReloadIdtr ENDP

AInvept PROC PUBLIC

    invept  rcx, oword ptr [rdx]
    jz ErrorWithStatus
    jc ErrorCodeFailed
    
    xor     rax, rax
    ret

ErrorWithStatus: 
    mov     rax, 1
    ret

ErrorCodeFailed:
    mov     rax, 2
    ret

AInvept ENDP

AInvVpid PROC
    invvpid rcx, oword ptr [rdx]
    jz      ErrorWithStatus
    jc      ErrorCodeFailed
    xor     rax, rax
    ret
    
ErrorWithStatus:
    mov     rax, 1
    ret

ErrorCodeFailed:
    mov     rax, 2
    ret

AInvVpid ENDP

END