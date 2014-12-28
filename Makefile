CC=gcc
LIBS=-lreadline -lpthread
EXEC=myshell
EXEC_SERVER=server
EXEC_CLIENT=client
CCFLAGS=-g -Wall

all: $(EXEC) $(EXEC_SERVER) $(EXEC_CLIENT)
.PHONY: all

$(EXEC): main.o cmd.o shell_fct.o 
	gcc $(CCFLAGS) -o  $(EXEC) main.o cmd.o shell_fct.o $(LIBS)

$(EXEC_SERVER): server.o cmd.o shell_fct.o
	gcc $(CCFLAGS) -o  $(EXEC_SERVER) server.o cmd.o shell_fct.o $(LIBS)

$(EXEC_CLIENT): client.o
	gcc $(CCFLAGS) -o  $(EXEC_CLIENT) client.o

cmd.o: cmd.c
	$(CC)  $(CCFLAGS) -o cmd.o -c cmd.c

shell_fct.o: shell_fct.c
	$(CC)  $(CCFLAGS) -o shell_fct.o -c shell_fct.c
 
server.o: server.c
	$(CC)  $(CCFLAGS) -o server.o -c server.c
 
main.o: main.c
	$(CC)  $(CCFLAGS) -o main.o -c main.c

client.o: client.c
	$(CC) $(CCFLAGS) -o client.o -c client.c
clean:
	rm -vf *.o
