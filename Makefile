# NAME: Jesse Joseph
# EMAIL: jjoseph@hmc.edu
# ID: 040161840

GXX=gcc
WARNING_FLAGS=-Wall -Wpedantic -Wextra

all: server.c server.h client.c client.h 
	make server
	make client

server: server.c server.h
	$(GXX) server.c -o server -lpthread $(WARNING_FLAGS)

client: client.c client.h
	$(GXX) client.c -o client -lpthread $(WARNING_FLAGS)

clean:
	-rm server
	-rm client
