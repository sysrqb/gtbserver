#include <mysql/mysql.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "cred.h"
#include "setDB.h"

#define STMT	"SELECT car FROM activeusers WHERE nightlyhash = ?"
//#define STMT	"SELECT nightlyhash FROM activeusers"
#define DEBUG 1
#define HASH_LEN	8

int checkhash(char * ptr2hash);
int execauth(char hash);
void closeall(MYSQL * mysql, MYSQL_STMT * stmt, MYSQL_RES * result);
MYSQL_STMT *mysqlinit(int *ret, MYSQL *myhandler, MYSQL_STMT *myssh);
int mysqlbindexec(int *ret, MYSQL_STMT *mystmthdler, MYSQL_BIND bind[], MYSQL_RES *myres);
