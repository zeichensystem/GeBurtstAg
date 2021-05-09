CC := clang

%.o: %.c
	$(CC) $< -Wall -o $@

math_divlut.h math_divlut.c: lutcalc 
	./lutcalc
	mv math_divlut.h ../source/math_divlut.h
	mv math_divlut.c ../data/math_divlut.c
