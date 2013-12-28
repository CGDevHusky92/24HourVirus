ASSIGNMENT = bug
FLAGS=-g

all: 
	clear
	gcc $(FLAGS) -c open.c close.c
	gcc $(FLAGS) -o tstWrappers tstWrappers.c open.o close.o
	gcc $(FLAGS) -o host host.c
	gcc $(FLAGS) -o virus virus.c
	cp virus seed
	printf '\xde\xad\xbe\xef' >> seed
	cat host >> seed

clean:
	rm -f ./*~
	rm -f ./*.o
	rm -f ./host
	rm -f ./seed
	rm -f ./virus
	rm -f ./tstWrappers
	rm -f ./bug.tgz

prepare:
	make clean
	gtar zcvf bug.tgz *

