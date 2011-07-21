#include "mysqlconn.h"

/*Function: mysqlinit
 * params:
 * ret: int*
 * myhandler: MYSQL*
 * myssh: MYSQL_STMT*
 * Establish connection with server and handlers
*/

MYSQL_STMT *mysqlinit(int *ret, MYSQL *myhandler, MYSQL_STMT *myssh){
	MYSQL_STMT *mystmthdler = myssh;
	if(DEBUG)
		printf("mysql_real_connect\n");
	//Establishes connection handler for communication with server
	// Credentials are defined in cred.h
	if((myhandler = mysql_real_connect(myhandler, HOST, USER, PASS, DB, MYPORT, U_S, C_F)) == NULL){
		fprintf(stderr, "Connection Error: %s\n", mysql_error(myhandler));
		exit(1);
	}

	if(DEBUG)
		printf("mysql_stmt_init\n");
	//Initialize mysql_stmt handler
	if((mystmthdler = mysql_stmt_init(myhandler)) == NULL){
		fprintf(stderr, "Hash Checking Error: MYSQL_STMT initg: %s\n", mysql_stmt_error(mystmthdler));
		exit(1);
	}
	*ret = 0;
	return mystmthdler;
}

/*Function: mysqlbindexec
 * params:
 * ret: int*
 * mystmthdler: MYSQL_STMT*
 * bind: MYSQL_BIND*
 * myres: MYSQL_RES*
 *
 * Bind and execute statement
*/

int mysqlbindexec(int *ret, MYSQL_STMT *mystmthdler, MYSQL_BIND bind[], MYSQL_RES *myres){
	if(DEBUG)
		printf("mysql_stmt_bind_param \n");
	//Bind hash to STMT
	if(mysql_stmt_bind_param(mystmthdler, bind)){
		fprintf(stderr, "Hash Checking Error: Binding: %s\n", mysql_stmt_error(mystmthdler));
		*ret = -4;
	}

	if(DEBUG)
		printf("mysql_num_fields\n");
	//Retrieve the number of columns returned in result set
	if(mysql_num_fields(myres) != 1){
		fprintf(stderr, "Hash Checking Error: Returned Fields: %s\n", mysql_stmt_error(mystmthdler));
		*ret = -4;
	}
  
	if(DEBUG)
		printf("mysq_stmt_execute\n");
	//Execute query
	if(mysql_stmt_execute(mystmthdler)){
		fprintf(stderr, "Hash Checking Error: Executing: %s\n", mysql_stmt_error(mystmthdler));
		*ret = -4;
	}
	return 0;
}

/*Function: checkhash
 * params:
 * hash: char*
 *
 * Establish connection with MySQL server
 * Query table for instance of ride-along with\
 *  specified hash
 * If query returns a result, then hash is valid\
 *  and authorization is given
 * If query does not return a result, then hash\
 *  is not valid and authorization fails
 */

int checkhash(char hashptr[]){
	int retval; //Used for error handling and for result set
	MYSQL * myhandler; //Used to establish connection with MySQL server
	MYSQL_STMT * mystmthdler; //Used with prepared statement
	MYSQL_RES * myres = NULL; //Returned by result metadata
	MYSQL_BIND bind[1]; //Binds hash to query
	unsigned long length[1];
	my_bool is_null[1];
	my_bool error[1];
	unsigned long num_rows;
	unsigned long hash_len;
	int *ret =&retval;
	
	if(DEBUG){
		printf("Hash: %s\n", hashptr);
		printf("mysql_init\n");
	}
	//Initiates MySQL handler; PARAM: NULL because myhandler is not allocated yet: SEGV
	if((myhandler = mysql_init(NULL)) == NULL){
		fprintf(stderr, "Init: Out of Memory:\n");
		exit(1);
	}

	mystmthdler = mysqlinit(ret, myhandler, mystmthdler);
	if(*ret < 0)
		return *ret;

	if(DEBUG)
		printf("mysql_stmt_prepare\n");
	//Prepare the provided statement STMT for execution by binding it to the handler
	if(mysql_stmt_prepare(mystmthdler, STMT, strlen(STMT))){
		fprintf(stderr, "Hash Checking Error: Preparing: %s\n", mysql_stmt_error(mystmthdler));
		exit(1);
	}

	if(DEBUG)
		printf("mysql_stmt_result_metadata \n");
	//Obtain resultset metadata
	myres = mysql_stmt_result_metadata(mystmthdler);
	if(DEBUG){
		if(myres == NULL)
			printf("myres is NULL\n");
		else
			printf("myres is not NULL\n");
	}
	
	if(DEBUG)
		printf("set bind struct values \n");
	//Zero-out handler
	memset(bind, 0, sizeof(bind));
	
	//Fill out struct
	bind[0].buffer_type = MYSQL_TYPE_STRING;
	bind[0].buffer = (char *)hashptr;
	bind[0].buffer_length = HASH_LEN;
	bind[0].length = &hash_len;

	hash_len = strlen(hashptr); //set length of hashso mysql knows how many characters are in the string

	if(DEBUG)
		printf("check to make sure hash is present: %s \n", hashptr);
		printf("Bound Hash: %s\n", bind[0].buffer);
	//if hash is null, set is_null == true
	if(!strncmp(hashptr, "", sizeof &hashptr)){
		fprintf(stderr, "Hash Checking Error: Hash Present Check\n");
		closeall(myhandler, mystmthdler, myres);
		return -1; //No hash
	}
	else{
		bind[0].is_null = (my_bool *)0;
	}
	
	mysqlbindexec(ret, mystmthdler, bind, myres);
	if(*ret < 0)
		return *ret;

	if(DEBUG)
		printf("reset bind struct values\n");
	//Bind result set
	memset(bind, 0, sizeof(bind));
	bind[0].buffer_type = MYSQL_TYPE_LONG;
	bind[0].buffer = (char *)&retval;
	bind[0].length = &length[0];
	bind[0].is_null = &is_null[0];
	bind[0].error = &error[0];

	if(DEBUG)
		printf("mysql_stmt_bind_result\n");
	//Bind result set to retrieve the returned rows
	if(mysql_stmt_bind_result(mystmthdler, bind)){
		fprintf(stderr, "Hash Checking Error: Binding Results: %s\n", mysql_stmt_error(mystmthdler));
		return -4;
	}

	if(DEBUG)
		printf("mysql_stmt_store_result\n");
	//Buffer all results
	if(mysql_stmt_store_result(mystmthdler)){
		fprintf(stderr, "Hash Checking Error: Binding Results: %s\n", mysql_stmt_error(mystmthdler));
		return -4;
	}

	if(DEBUG)
		printf("mysql_stmt_num_rows\n");
	//Check the number of rows returned from query
	// If >1, fail authentication
	if((num_rows = mysql_stmt_num_rows(mystmthdler)) > 1){
		closeall(myhandler, mystmthdler, myres);
		return -2; //Multiple results returned...should not be possible
	}
		
	if(DEBUG)
		printf("mysql_stmt_fetch\n");
		printf("num_rows = %lu \n", num_rows);
	int rows = 0;
	int fetchret = 0;
	//Fetch the next row in the result set
	while(!(fetchret = mysql_stmt_fetch(mystmthdler))){
		printf("num_rows: %lu\t rows: %d\n", num_rows, rows);
		if(++rows > num_rows){
			//TODO Return invalid login
			closeall(myhandler, mystmthdler, myres);
			return -2;
		}
	}

	closeall(myhandler, mystmthdler, myres);
	if(DEBUG){
		printf("Car ID: %d\n", retval);
		printf("Rows: %d\n", rows);
		printf("fetret: %d\n", fetchret);
	}
	
	return retval;
}

void closeall(MYSQL * mysql, MYSQL_STMT * stmt, MYSQL_RES * result){
	mysql_free_result(result);
	if(mysql_stmt_close(stmt)){
		fprintf(stderr, "Hash Checking Error: Closing statement: %s\n", mysql_stmt_error(stmt));
		exit(1);
	}
		
	mysql_close(mysql);
	mysql_library_end();
}