morse: main.c
	gcc main.c -lasound -lm -o morse

clean:
	rm -f morse
