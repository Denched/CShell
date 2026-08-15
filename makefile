
simpleshell: src/simpleshell.c src/process.c src/utility.c src/simpleshell.h
	mkdir -p bin
	gcc -Wall src/simpleshell.c src/process.c src/utility.c -o bin/simpleshell

clean:
	rm -f bin/simpleshell

