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

#ifndef THREADING_HPP
#define THREADING_HPP

#include "gtbcommunication.hpp"


extern "C"
void *(gtbAccept_c)(void * instance);


extern "C"
void *(for_communication)(void * instance);


extern "C"
void *(for_watchdog)(void * instance);


extern pthread_t createAcceptThread(GTBCommunication * aComm,
                                           pthread_attr_t * attr);

extern pthread_t createCommThread(GTBCommunication * aComm, 
                                         pthread_attr_t * attr);

extern pthread_t createWDogThread(GTBCommunication * aComm,
                                         pthread_attr_t * attr);
#endif
