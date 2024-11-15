TESTS = $(wildcard *.c)
all:	$(TESTS:.c=-dyn)
%-dyn:	%.c
	${GCC} $< -o $@

clean:
	rm *-dyn
	