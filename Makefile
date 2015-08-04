CC=gcc
CFLAGS = -g 
# uncomment this for SunOS
# LIBS = -lsocket -lnsl

all: sendfile recvfile

sendfile: sendfile.o 
	$(CC) -o sendfile sendfile.o $(LIBS)

recvfile: recvfile.o 
	$(CC) -o recvfile recvfile.o $(LIBS)

sendfile.o: sendfile.c

recvfile.o: recvfile.c

clean:
	rm -f sendfile recvfile sendfile.o recvfile.o 
