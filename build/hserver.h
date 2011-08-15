/*
 * Copyright 2003-2011 Jeffrey K. Hollingsworth
 *
 * This file is part of Active Harmony.
 *
 * Active Harmony is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Active Harmony is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Active Harmony.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef __HSERVER_H__
#define __HSERVER_H__

/***
 *
 * include system headers
 *
 ***/
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string>
#include <map>

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <assert.h>
#include <sstream>
#include <set>
// tcl/tk
#include <tcl.h>
#include <tk.h>

/***
 *
 * include user defined headers
 *
 ***/
#include "hutil.h"
#include "hmesgs.h"
#include "hsockutil.h"

/***
 *
 * define macros
 *
 ***/
#define BUFF_SIZE 1024

#endif /* __HSERVER_H__ */
