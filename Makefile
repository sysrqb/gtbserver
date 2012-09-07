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

obj = main.o sqlconn.o gtbcommunication.o threading.o jsoncpp.o
SRC=src/
TEST=test/
INCLUDE=include/
LIB=lib/
LINKEROPTS = -Wl -lmysqlcppconn -lpthread -lnettle -lcurl 
GNUTLS=/usr/lib/libtasn1.a /usr/lib/libnettle.a /usr/lib/libgnutls.a
PKGCOPTS = `pkg-config gnutls --cflags --libs protobuf`
CC=clang++ -ggdb -Wall -I$(INCLUDE)
GMOCK=../gmock-1.6.0
GTEST=$(GMOCK)/gtest
TESTING_INCLUDES=-I$(GTEST)/include -I$(GTEST) -I$(GMOCK)/include -I$(GMOCK)

gtb : gtbserver.o
	$(CC) $(LINKEROPTS) -o gtbserver $(obj) communication.pb.o patron.pb.o $(PKGCOPTS)
	#doxygen docs/gtbdoxygen.conf

static : gtbserver.o
	$(CC) -v -L$(LIB) $(LINKEROPTS) -o gtbserver $(obj) communication.pb.o patron.pb.o $(PKGCOPTS)
	#ar -rv gtb main.o threading.o sqlconn.o gtbcommunication.o communication.pb.o patron.pb.o jsoncpp.o
	#ar -rv libdeps $(GNUTLS) 
	#ar -srv gtbserver-ar gtb libdeps
	#$(CC) -static -o gtbserver gtbserver-ar

gtbserver.o : $(SRC)main.cc $(SRC)threading.cc $(SRC)sqlconn.cc $(SRC)gtbcommunication.cc $(SRC)communication.pb.cc $(SRC)jsoncpp.cpp
	$(CC) -c $(SRC)main.cc $(SRC)threading.cc $(SRC)sqlconn.cc $(SRC)gtbcommunication.cc $(SRC)communication.pb.cc $(SRC)patron.pb.cc $(SRC)jsoncpp.cpp

communication.pb.cc : patron.pb.cc
	./protobuf/bin/protoc -I$(SRC) -Iinclude $(SRC)communication.proto --cpp_out=$(SRC) --java_out=$(SRC)
	mv $(SRC)communication.pb.h $(INCLUDE)

patron.pb.cc : 
	./protobuf/bin/protoc -I$(SRC) $(SRC)patron.proto --cpp_out=$(SRC) --java_out=$(SRC)
	mv $(SRC)patron.pb.h $(INCLUDE)

docs:
	doxygen docs/gtbdoxygen.conf


##############
## TESTING ###
##############

test : gtest-all.cc
	$(CC) $(TESTING_INCLUDES) $(LINKEROPTS) -o gtb-test-all $(TEST)gtbcommunicationtest.cc $(TEST)externalfunctions.cc $(SRC)threading.cc $(TEST)gtb-test-main.cc $(TEST)gtb-test-curr.cc $(TEST)gtb-test-comm.cc $(SRC)sqlconn.cc $(SRC)gtbcommunication.cc $(SRC)patron.pb.cc $(SRC)communication.pb.cc $(SRC)jsoncpp.cpp libgmock.a $(PKGCOPTS)

testcomm : gtest-all.cc
	$(CC) $(TESTING_INCLUDES) $(LINKEROPTS) -o gtb-test-comm $(TEST)gtbcommunicationtest.cc $(TEST)externalfunctions.cc $(SRC)threading.cc $(TEST)gtb-test-comm.cc $(SRC)sqlconn.cc $(SRC)gtbcommunication.cc $(SRC)patron.pb.cc $(SRC)communication.pb.cc $(SRC)jsoncpp.cpp libgmock.a $(PKGCOPTS)

testcurr : gtest-all.cc
	$(CC) $(TESTING_INCLUDES) $(LINKEROPTS) -o gtb-test-curr $(TEST)gtbcommunicationtest.cc $(TEST)externalfunctions.cc $(SRC)threading.cc $(TEST)gtb-test-curr.cc $(SRC)sqlconn.cc $(SRC)gtbcommunication.cc $(SRC)patron.pb.cc $(SRC)communication.pb.cc $(SRC)jsoncpp.cpp libgmock.a $(PKGCOPTS)

sigtest : gtest-all.cc
	$(CC) $(TESTING_INCLUDES) $(LINKEROPTS) -o gtb-test-main $(TEST)gtbcommunicationtest.cc $(TEST)externalfunctions.cc $(SRC)threading.cc $(TEST)gtb-test-main.cc libgmock.a $(PKGCOPTS)

gtest-all.cc : gtest_main.o
	$(CC) $(TESTING_INCLUDES) -c $(GTEST)/src/gtest-all.cc
	$(CC) $(TESTING_INCLUDES) -c $(GMOCK)/src/gmock-all.cc
	ar -rv libgmock.a gtest-all.o gtest_main.o gmock-all.o

gtest_main.o : 
	$(CC) -I$(GTEST)/include -I$(GTEST) -c $(GTEST)/src/gtest_main.cc

verify: verifyingcert.c
	gcc -ggdb -Wall $(LINKEROPTS) -o verify verifyingcert.c

testgpgjson :
	$(CC) $(LINKEROPTS) -o gpgjsontest $(TEST)gpgjsontest.cc $(SRC)patron.pb.cc $(SRC)communication.pb.cc $(SRC)jsoncpp.cpp $(PKGCOPTS)

testjsonparse :
	$(CC) $(LINKEROPTS) -o testjsonparse $(TEST)testjsonparse.cc $(SRC)jsoncpp.cpp $(PKGCOPTS)

cleantest:
	rm gtb-test-*

.Phony : clean
clean :
	rm gtbserver $(obj) *.o 
