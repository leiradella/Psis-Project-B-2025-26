CC 				:= gcc
CFLAGS 			:= -g -Wall -Wextra -Werror -pedantic
CONFIGFLAGS 	:= -lconfig
SDLFLAGS		:= -lSDL2 -lSDL2_image -lSDL2_ttf -lSDL2_gfx
ZMQFLAGS		:= -lzmq
PROTOFLAGS		:= -lprotobuf-c
MATHFLAGS		:= -lm
CLIENTDIR 		:= ./trash-ship-client/
COMMONDIR		:= ./Common/
CLIENTDIRSRC 	:= new-universe-client.c 
CLIENTCOMMONSRC := graceful-exit.c display.c new-Communication.c msgB.pb-c.c universe-data.c
PROTOSRC		:= msgB.proto
PROTOOUT		:= msgB.pb-c.h msgB.pb-c.c

all: new-universe-client test-server proto

new-universe-client: $(addprefix $(CLIENTDIR), $(CLIENTDIRSRC))
	$(CC) $(CFLAGS)	-o $(CLIENTDIR)$@	\
	$(addprefix $(CLIENTDIR), $(CLIENTDIRSRC))	\
	$(addprefix $(COMMONDIR), $(CLIENTCOMMONSRC))	\
	$(CONFIGFLAGS)	$(SDLFLAGS)	$(ZMQFLAGS) $(PROTOFLAGS) $(MATHFLAGS)	

test-server:	test-server.c
	$(CC) $(FLAGS) -o $@	\
	$@.c 

.PHONY: proto 
proto: $(addprefix $(COMMONDIR), $(PROTOOUT)) 
$(addprefix $(COMMONDIR), $(PROTOOUT)): $(COMMONDIR)msgB.proto
	protoc --c_out=. $<

clean: clean_test-server clean_new-universe-client clean_proto 

clean_test-server:
	rm test-server

clean_new-universe-client: 
	rm $(CLIENTDIR)new-universe-client

clean_proto: 
	rm $(addprefix $(COMMONDIR), $(PROTOOUT))


