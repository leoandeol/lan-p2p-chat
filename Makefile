all : clean main

main :
	gcc main.c -o main -O3 -lpthread -lncurses -Wall -Wextra -Werror

clean :
	rm -f main > /dev/null
