# NAME: Jesse Joseph
# EMAIL: jjoseph@hmc.edu
# ID: 040161840

GXX=gcc
WARNING_FLAGS=-Wall -Wpedantic -Wextra

server: server.c server.h
	$(GXX) server.c -o server -lpthread $(WARNING_FLAGS)

clean:
	-rm server

