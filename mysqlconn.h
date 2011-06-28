#include <mysql/mysql.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "cred.h"

#define DEBUG 1

int checkhash(char * hashptr);
int execauth(char hash);
void closeall(MYSQL * mysql, MYSQL_STMT * stmt, MYSQL_RES * result);
