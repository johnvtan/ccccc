all:
	gcc -g -O0 -Wall -Wextra -o COMPILERBABY main.c list.c string.c tokenize.c parse.c introspect.c

clean:
	rm -rf COMPILERBABY
