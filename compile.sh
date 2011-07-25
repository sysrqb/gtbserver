#!/bin/sh

clang -Wl,-L/usr/lib/mysql,-lmysqlclient -ggdb comm.c gtbserver.c mysqlconn.c `pkg-config gnutls --cflags --libs`
