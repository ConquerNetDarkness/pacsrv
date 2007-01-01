# pacsrv
# Clinton Webb
# June 18, 2005.


all: pacsrvclient pacsrvd 


D_OBJS=pacsrvd.o config.o logger.o \
	baseserver.o baseclient.o \
	server.o client.o \
	network.o node.o \
	serverlist.o serverinfo.o address.o \
	filelist.o fileinfo.o
	
D_LIBS=-lpthread -ldevplus-thread -ldevplus-main -ldevplus

FLAGS=-g -Wall 
OFLAGS=

H_fileinfo=fileinfo.h
H_filelist=filelist.h $(H_fileinfo)
H_logger=logger.h
H_common=common.h
H_config=config.h
H_baseclient=baseclient.h
H_baseserver=baseserver.h
H_client=client.h $(H_common) $(H_baseclient)
H_address=address.h $(H_config)
H_node=node.h $(H_baseclient) $(H_address) $(H_fileinfo)
H_serverinfo=serverinfo.h $(H_address) 
H_serverlist=serverlist.h $(H_serverinfo)
H_network=network.h $(H_baseserver) $(H_node) $(H_serverlist) $(H_filelist)
H_server=server.h $(H_baseserver) $(H_client) $(H_network)


pacsrvd: $(D_OBJS)
	g++ -o pacsrvd $(D_OBJS) $(D_LIBS) $(OFLAGS)

pacsrvd.o: pacsrvd.cpp $(H_server) $(H_config) $(H_logger)
	g++ -c -o pacsrvd.o pacsrvd.cpp $(FLAGS)

server.o: server.cpp $(H_server) $(H_config)
	g++ -c -o server.o server.cpp  $(FLAGS)

baseserver.o: baseserver.cpp $(H_baseserver)
	g++ -c -o baseserver.o baseserver.cpp  $(FLAGS)

baseclient.o: baseclient.cpp $(H_baseclient)
	g++ -c -o baseclient.o baseclient.cpp  $(FLAGS)

client.o: client.cpp $(H_client) $(H_config) $(H_logger)
	g++ -c -o client.o client.cpp  $(FLAGS)

network.o: network.cpp $(H_network) $(H_config) $(H_logger) $(H_address)
	g++ -c -o network.o network.cpp  $(FLAGS)

node.o: node.cpp $(H_node) $(H_config) $(H_logger)
	g++ -c -o node.o node.cpp  $(FLAGS)

config.o: config.cpp $(H_config)
	g++ -c -o config.o config.cpp  $(FLAGS)

logger.o: logger.cpp $(H_logger)
	g++ -c -o logger.o logger.cpp  $(FLAGS)

address.o: address.cpp $(H_address)
	g++ -c -o address.o address.cpp  $(FLAGS)

serverinfo.o: serverinfo.cpp $(H_serverinfo)
	g++ -c -o serverinfo.o serverinfo.cpp  $(FLAGS)

serverlist.o: serverlist.cpp $(H_serverlist)
	g++ -c -o serverlist.o serverlist.cpp  $(FLAGS)

filelist.o: filelist.cpp $(H_filelist)
	g++ -c -o filelist.o filelist.cpp  $(FLAGS)

fileinfo.o: fileinfo.cpp $(H_fileinfo) $(H_common)
	g++ -c -o fileinfo.o fileinfo.cpp  $(FLAGS)


pacsrvclient: pacsrvclient.cpp $(H_common)				
	g++ -o pacsrvclient pacsrvclient.cpp $(FLAGS) $(D_LIBS)



clean: 
	@-rm $(D_OBJS) 2>/dev/null
	@-rm pacsrvd-log*

