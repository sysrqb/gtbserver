# GUARD the Bridge
# Copyright (C) 2012  Matthew Finkel <Matthew.Finkel@gmail.com>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as
# published by the Free Software Foundation, either version 3 of the
# License, or any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

obj = gtbserver.o sqlconn.o gtbcommunication.o communication.pb.o patron.pb.o
SRC=src/
LIBS=./include/
PLIBS=./protobuf/include/
MYSQLOPTS = -Wl,-lmysqlcppconn,-lpthread,-lgnutls
PKG_CONFIG_PATH=$(HOME)/repos/gtbserver/protobuf/lib/pkgconfig
PKGCOPTS = `pkg-config gnutls --cflags --libs `
CC=clang++ -ggdb -I$(PLIBS) -I$(LIBS)

gtb : $(obj)
	$(CC) $(MYSQLOPTS) -o gtbserver $(obj) $(PKGCOPTS)

gtbserver.o : $(SRC)gtbserver.c $(SRC)sqlconn.cc $(SRC)gtbcommunication.cc communication.pb.cc 
	$(CC) -c $(SRC)sqlconn.cc $(SRC)gtbcommunication.cc $(SRC)gtbserver.c $(SRC)communication.pb.cc $(SRC)patron.pb.cc

communication.pb.cc : patron.pb.cc
	./protobuf/bin/protoc -I$(SRC) -Iinclude $(SRC)communication.proto --cpp_out=$(SRC) --java_out=$(SRC)
	mv $(SRC)communication.pb.h $(LIBS)

patron.pb.cc : 
	./protobuf/bin/protoc -I$(SRC) $(SRC)patron.proto --cpp_out=$(SRC) --java_out=$(SRC)
	mv $(SRC)patron.pb.h $(LIBS)


.Phony : clean
clean :
	rm gtbserver $(obj)
