# NAME: Jesse Joseph
# EMAIL: jjoseph@hmc.edu
# ID: 040161840

GXX=gcc
WARNING_FLAGS=-Wall -Wpedantic -Wextra

all: server.c server.h client.c client.h 
	make server
	make client

server: server.c server.h
	$(GXX) server.c -o lab1b-server -lpthread -lmcrypt $(WARNING_FLAGS)

client: client.c client.h
	$(GXX) client.c -o lab1b-client -lmcrypt $(WARNING_FLAGS)

clean:
	-rm lab1b-server
	-rm lab1b-client
	-rm lab1b-040161840.tar.gz

dist:
	tar -czvf lab1b-040161840.tar.gz client.c client.h server.c server.h Makefile README my.key

