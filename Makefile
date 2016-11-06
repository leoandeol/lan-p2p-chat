all : clean main

main :
	gcc main.c -o main -O3 -lpthread -Wall -Wextra -Werror

clean :
	rm main > /dev/null
