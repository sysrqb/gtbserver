#!/bin/sh


#clang -Wl,-L/usr/lib/mysql,-lmysqlclient -ggdb comm.c gtbserver.c mysqlconn.c `pkg-config gnutls --cflags --libs`

obj = gtbserver.o sqlconn.o gtbcommunication.o
MYSQLOPTS = -Wl,-lmysqlcppconn,-lpthread,-lgnutls
TLSOPTS = `pkg-config gnutls --cflags --libs`
CC=clang++ -ggdb

gtb : $(obj)
	$(CC) $(MYSQLOPTS) -o gtbserver $(obj) $(TLSOPTS)

gtbserver.o : gtbserver.c sqlconn.cc gtbcommunication.cc 
	$(CC) -c sqlconn.cc gtbcommunication.cc gtbserver.c


.Phony : clean
clean :
	rm gtbserver $(obj)
