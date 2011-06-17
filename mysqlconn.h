#include <mysql/mysql.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "cred.h"

int checkhash(char hash);
int execauth(char hash);
void closeall(MYSQL * mysql, MYSQL_STMT * stmt, MYSQL_RES * result);
