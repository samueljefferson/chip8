all:
	bash -c "resize -s 32 64"
	'clear'
	gcc -o chip8 chip8.c -lncurses -lpthread

clean:
	bash -c "resize -s 24 80"
	'clear'
	rm chip8
