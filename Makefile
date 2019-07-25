all: netshell.c
	gcc -Wall netshell.c -o netshell

.PHONE: clean

clean:
	rm *.o netshell

