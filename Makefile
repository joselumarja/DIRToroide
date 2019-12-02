DIROBJ := obj/
DIREXE := exec/
DIRHEA := include/
DIRSRC := src/
DIRMAIN := ./
DIRUTILITIES := $(DIRHEA)utilities.o

CFLAGS := -I$(DIRHEA) -c -Wall -ansi -g -lm
IFLAGS :=  -c -Wall -ansi -g -lm
LDLIBS := -lpthread -lrt
CC := gcc
MPI := mpicc
MPIRUN := mpirun

all : dirs utilities Toroid

dirs:
	mkdir -p $(DIROBJ) $(DIREXE)

utilities: $(DIRHEA)utilities.c
	$(CC) $(IFLAGS) -o $(DIRUTILITIES) $^ $(LDLIBS)
	
Toroid: $(DIROBJ)Toroide.o 
	$(MPI) $(DIRUTILITIES) $(DIROBJ)Toroide.o -o $(DIRMAIN)Toroid -lm
	 
$(DIROBJ)%.o: $(DIRSRC)%.c
	$(MPI) $(CFLAGS) $^ -o $@

test:
	$(MPIRUN) -n 16 ./Toroid ./datos.dat 4

clean : 
	rm -rf *~ core $(DIROBJ) $(DIREXE) $(DIRHEA)*~ $(DIRSRC)*~ $(DIRUTILITIES) 
