all: netshell shellcode

netshell: netshell.c
	gcc -Wall $^ -o netshell

shellcode: netshellcode.o
	ld -m elf_i386 $^ -o netshellcode
	strip -R .note.gnu.property netshellcode

netshellcode.o: netshellcode.asm
	nasm -f elf $^

.PHONE: clean

clean:
	rm *.o netshell netshellcode

