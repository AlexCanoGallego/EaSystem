CC = gcc -Wall -Wextra -g

OFILES = IluvatarSon.o SharedFunc.o Commands.o Connection.o Arda.o Send.o Mailbox.o
TARGET = IluvatarSon Arda

all: clean IluvatarSon Arda

IluvatarSon: IluvatarSon.o SharedFunc.o Commands.o Connection.o Send.o Mailbox.o
	$(CC) -pthread -o IluvatarSon IluvatarSon.o SharedFunc.o Connection.o Commands.o Send.o Mailbox.o -lrt

Arda: Arda.o SharedFunc.o Commands.o Connection.o
	$(CC) -pthread -o Arda Arda.o SharedFunc.o Connection.o Commands.o -lrt

SharedFunc.o:
	$(CC) -c SharedFunc.c

Commands.o:
	$(CC) -c Commands.c

Connection.o:
	$(CC) -c Connection.c
 
Send.o:
	$(CC) -c Send.c

Mailbox.o:
	$(CC) -c Mailbox.c
 
clean:
	rm -f $(OFILES) $(TARGET) *~
