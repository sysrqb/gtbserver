#!/bin/sh

clang -Wl,-L/usr/lib/mysql,-lmysqlclient gtbserver.c mysqlconn.c
