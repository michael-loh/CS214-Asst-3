all: DUMBclient.c client.o DUMBserver.c server.o
	gcc DUMBclient.c -ggdb -o DUMBclient client.o
	gcc DUMBserver.c -pthread -ggdb -o DUMBserve server.o
client: DUMBclient.c client.o
	gcc DUMBclient.c -g -o DUMBclient client.o
client.o:
	gcc client.c -c
serve: DUMBserver.c server.o
	gcc DUMBserver.c -pthread -g -o DUMBserve server.o
server.o:
	gcc server.c -c
clean:
	rm DUMBclient; rm client.o; rm DUMBserve; rm server.o
