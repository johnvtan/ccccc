all:
	gcc -g -O0 -Wall -Wextra -o COMPILERBABY main.c list.c string.c tokenize.c parse.c introspect.c asm.c map.c output.c

debug:
	gcc -g -O0 -Wall -Wextra -DDEBUG -o COMPILERBABY main.c list.c string.c tokenize.c parse.c introspect.c asm.c map.c output.c

clean:
	rm -rf COMPILERBABY
