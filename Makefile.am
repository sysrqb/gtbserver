bin_PROGRAMS = gtbserver
gtbserver_SOURCES = \
  src/threading.cc \
  src/main.cc \
  src/sqlconn.cc \
  src/gtbcommunication.cc \
  src/patron.pb.cc \
  src/communication.pb.cc \
  src/jsoncpp.cpp
dist_doc_DATA = README TODO
AM_CPPFLAGS = -Iinclude/

test : all
	$(CXX) $(LIBS) $(AM_CPPFLAGS) -lgmock_main -o gtb-test-all test/gtbcommunicationtest.cc \
  test/externalfunctions.cc threading.o test/gtb-test-main.cc \
  test/gtb-test-curr.cc test/gtb-test-comm.cc sqlconn.o gtbcommunication.o \
  patron.pb.o communication.pb.o jsoncpp.o

testcomm : 
	$(CC) $(LIBS) -o gtb-test-comm test/gtbcommunicationtest.cc \
  test/externalfunctions.cc threading.o test/gtb-test-comm.cc sqlconn.o \
  gtbcommunication.o patron.pb.o communication.pb.o jsoncpp.o libgmock.a

testcurr : 
	$(CC) $(TESTING_INCLUDES) $(LINKEROPTS) -o gtb-test-curr test/gtbcommunicationtest.cc test/externalfunctions.cc $(SRC)threading.cc test/gtb-test-curr.cc $(SRC)sqlconn.cc $(SRC)gtbcommunication.cc $(SRC)patron.pb.cc $(SRC)communication.pb.cc $(SRC)jsoncpp.cpp libgmock.a $(PKGCOPTS)

sigtest : 
	$(CC) $(TESTING_INCLUDES) $(LINKEROPTS) -o gtb-test-main test/gtbcommunicationtest.cc test/externalfunctions.cc $(SRC)threading.cc test/gtb-test-main.cc libgmock.a $(PKGCOPTS)

gtest-all.cc : 
	$(CC) $(TESTING_INCLUDES) -c $(GTEST)/src/gtest-all.cc
	$(CC) $(TESTING_INCLUDES) -c $(GMOCK)/src/gmock-all.cc
	ar -rv libgmock.a gtest-all.o gtest_main.o gmock-all.o

gtest_main.o : 
	$(CC) -I$(GTEST)/include -I$(GTEST) -c $(GTEST)/src/gtest_main.cc


