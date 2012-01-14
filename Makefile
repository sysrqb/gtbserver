#!/bin/sh


#clang -Wl,-L/usr/lib/mysql,-lmysqlclient -ggdb comm.c gtbserver.c mysqlconn.c `pkg-config gnutls --cflags --libs`

obj = gtbserver.o mysqlconn.o gtbcommunication.o
MYSQLOPTS = -Wl,-L/usr/lib/mysql,-lmysqlclient,-lmysqlcppconn,-lpthread,-lgnutls
TLSOPTS = `pkg-config gnutls --cflags --libs`
CC=clang++ -ggdb

gtb : $(obj)
	$(CC) $(MYSQLOPTS) -o gtbserver $(obj) $(TLSOPTS)

gtbserver.o : gtbserver.c mysqlconn.c gtbcommunication.cc 
	$(CC) -c mysqlconn.c gtbcommunication.cc gtbserver.c


.Phony : clean
clean :
	rm gtbserver $(obj)
