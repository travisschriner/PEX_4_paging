ARCH := $(shell getconf LONG_BIT)


CMD_64 := gcc PEX4Template.o -o PEX4 -Wall

CMD := $(CMD_$(ARCH))

PEX4: PEX4Template.o   makefile
	$(CMD)

PEX4Template.o: PEX4Template.c byutr.h structures.h makefile 
	gcc -c PEX4Template.c

clean:
	-rm PEX4Template.o
	-rm *~
	-rm PEX4
	clear

