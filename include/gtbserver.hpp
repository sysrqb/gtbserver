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

#include "gtbcommunication.hpp"

extern "C" {
  void sigchld_handler (int);
  void initIncomingCon(int *);
}


/*Return Values:
 4: Error MySQL query prep
 3: reqbuf did not equal the value it was supposed to
 2: the hash query returned more than one result (should not be possible) - invalid login
 1: No hash was povided by client
 0: completed successfully
*/
