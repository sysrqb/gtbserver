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

obj = gtbserver.o sqlconn.o gtbcommunication.o
SRC=src/
INCLUDE=include/
LINKEROPTS = -Wl,-lmysqlcppconn,-lpthread,-lgnutls
PKGCOPTS = `pkg-config gnutls --cflags --libs protobuf`
CC=clang++ -ggdb -I$(INCLUDE)

gtb : $(obj)
	$(CC) $(LINKEROPTS) -o gtbserver $(obj) communication.pb.o patron.pb.o $(PKGCOPTS)

gtbserver.o : $(SRC)gtbserver.c $(SRC)sqlconn.cc $(SRC)gtbcommunication.cc communication.pb.cc 
	$(CC) -c $(SRC)sqlconn.cc $(SRC)gtbcommunication.cc $(SRC)gtbserver.c $(SRC)communication.pb.cc $(SRC)patron.pb.cc

communication.pb.cc : patron.pb.cc
	./protobuf/bin/protoc -I$(SRC) -Iinclude $(SRC)communication.proto --cpp_out=$(SRC) --java_out=$(SRC)
	mv $(SRC)communication.pb.h $(INCLUDE)

patron.pb.cc : 
	./protobuf/bin/protoc -I$(SRC) $(SRC)patron.proto --cpp_out=$(SRC) --java_out=$(SRC)
	mv $(SRC)patron.pb.h $(INCLUDE)


.Phony : clean
clean :
	rm gtbserver $(obj) communication.pb.o patron.pb.o
