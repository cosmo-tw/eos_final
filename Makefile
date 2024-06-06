all: server client panel

server: server.c
	gcc -g -o server server.c -lgpiod

client: client.c
	gcc -o client client.c

panel: panel.c
	gcc -o panel panel.c



clean:
	rm -f server client panel
