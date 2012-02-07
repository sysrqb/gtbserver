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
TEST=test/
INCLUDE=include/
LINKEROPTS = -Wl,-lmysqlcppconn,-lpthread,-lgnutls
PKGCOPTS = `pkg-config gnutls --cflags --libs protobuf`
CC=clang++ -ggdb -I$(INCLUDE)
GTEST=../gtest

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


##############
## TESTING ###
##############

test : gtest-all.cc
	$(CC) -I$(GTEST)/include -I$(GTEST) $(LINKEROPTS) -o gtb-test-curr $(TEST)gtb-test-curr.cc $(SRC)sqlconn.cc $(SRC)gtbcommunication.cc $(SRC)patron.pb.cc $(SRC)communication.pb.cc libgtest.a $(PKGCOPTS)

gtest-all.cc : gtest_main.o
	$(CC) -I$(GTEST)/include -I$(GTEST) -c $(GTEST)/src/gtest-all.cc
	ar -rv libgtest.a gtest-all.o gtest_main.o

gtest_main.o : 
	$(CC) -I$(GTEST)/include -I$(GTEST) -c $(GTEST)/src/gtest_main.cc
	

cleantest:
	rm gtb-test-*

.Phony : clean
clean :
	rm gtbserver $(obj) communication.pb.o patron.pb.o
