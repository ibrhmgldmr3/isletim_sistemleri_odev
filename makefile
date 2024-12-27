program: derle main

derle:
	gcc -I ./include/ -o ./lib/main.o -c ./src/main.c
	gcc -o ./bin/Main ./lib/main.o


main:
	./bin/Main
