#include "setDB.h"

int initDB(){
	int retval;
/*Clear Table&*/
	if((retval = clearActTable())){ //Drops ActiveUsers table and
		//TODO	     //recreates it so we can
			     //populate it without duplicates
	}
	if((retval = popActUsers())){ //Gets members signed up for the
		//TODO       //night from website DB and fills
		       //in necesary info
	}
/*---------*/

	int results;

	if((retval = getNumOfRes(&results))){
		//TODO
	}
	int i;
	for(i = 0; i<results; i++){
		char[] hash;
		if((retval = computehash(i, &hash))){ //computes hash for (i-1)th member in DB
			//TODO
		}
		if((retval = sethash(i, &hash))){ //Insert hash per uid
			//TODO
		}
	}
	return 0;
}
		
