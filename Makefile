all : clean main

debug : clean db

main :
	gcc main.c -o main -O3 -lpthread -lncurses -std=c99

db :
	gcc main.c -o main -O3 -lpthread -lncurses -Werror -Wall -Wextra -pedantic -Wcast-align -Wcast-qual  -Wdisabled-optimization -Wformat=2 -Winit-self -Wlogical-op -Wmissing-include-dirs -Wredundant-decls -Wshadow -Wundef -Wno-unused -Wno-variadic-macros -Wno-parentheses -std=c99

clean :
	rm -f main > /dev/null
