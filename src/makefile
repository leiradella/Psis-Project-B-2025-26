CC 				:= gcc
CFLAGS 			:= -g -Wall -Wextra -Werror -pedantic
CONFIGFLAGS 	:= -lconfig
CLIENTDIR 		:= ./trash-ship-client
COMMONDIR		:= ./Common
CLIENTDIRSRC 	:= new-universe-client.c 
CLIENTCOMMONSRC := temp
PROTOSRC		:= msgB.proto
PROTOOUT		:= msgB.pb-c.h msgB.pb-c.c

all: new-universe-client msgB.pb-c

new-universe-client: $(addprefix $(CLIENTDIR)/, $(CLIENTDIRSRC))
	$(CC) $(cflags) $(addprefix $(CLIENTDIR)/, $(CLIENTDIRSRC)) -o $(CLIENTDIR)/$@ \
		$(CONFIGFLAGS)

msgB.pb-c: $(Common)/msgB.proto
	protoc --c_out=. $(Common)/msgB.proto

clean: clean_msgB.pb-c

clean_new-universe-client: 
	rm $(CLIENTDIR)/new-universe-client

clean_msgB.pb-c: 
	rm $(addprefix $(COMMONDIR)/, $(PROTOOUT))


