#include <mysql/mysql.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "cred.h"

#define STMT	"SELECT car FROM activeusers WHERE nightlyhash = ?"
//#define STMT	"SELECT nightlyhash FROM activeusers"
#define DEBUG 1
#define HASH_LEN	8

int checkhash(char * ptr2hash);
int execauth(char hash);
void closeall(MYSQL * mysql, MYSQL_STMT * stmt, MYSQL_RES * result);
