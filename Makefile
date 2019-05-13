CC=cc
CFLAGS= -I/global/homes/j/jdagdele/project/clusters/graham/igraph_local/include/ -L/global/homes/j/jdagdele/project/clusters/graham/igraph_local/lib/ -ligraph -lgmp -I/usr/common/software/gsl/2.1/intel/include -L/usr/common/software/gsl/2.1/intel/lib -lgsl -lgslcblas -lstdc++ -std=c99

serial: serial.o
	$(CC)  serial.o -o serial $(CFLAGS)

serial.o: serial.c
	$(CC) -c serial.c $(CFLAGS) 

parallel: parallel.o
	$(CC)  parallel.o -o parallel $(CFLAGS) -fopenmp

parallel.o: parallel.c
	$(CC) -c parallel.c $(CFLAGS) -fopenmp


