/*
 * GUARD the Bridge
 * Copyright (C) 2012  Matthew Finkel <Matthew.Finkel@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

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
		
