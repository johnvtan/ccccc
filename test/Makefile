all: dir list map string

dir:
	mkdir -p bin

list:
	gcc -Wall -Wextra -o bin/test_list -I../ ../list.c test_list.c

map:
	gcc -Wall -Wextra -o bin/test_map -I../ ../map.c ../list.c ../string.c test_map.c

string:
	gcc -Wall -Wextra -o bin/test_string -I../ ../string.c test_string.c

clean:
	rm -rf bin
