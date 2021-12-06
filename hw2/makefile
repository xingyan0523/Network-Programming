all: ser cli

ser: ser.c tic_tac_toe.c tic_tac_toe.h
	gcc -g ser.c tic_tac_toe.c tic_tac_toe.h -o ser

cli: cli.c
	gcc -g cli.c -o cli


clean:
	rm cli
	rm ser