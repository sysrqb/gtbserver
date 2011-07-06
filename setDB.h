#include "mysqlconn.h"
#include <openssl/sha.h>

void initDB();
int clearActTable();
int popActUsers();
int getNumOfRes(int *results);
int computehash(int i, char[] hash);
int sethash(int i, char[] hash);
