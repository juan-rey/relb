/*
   utiles.h: utiles header file.

   Copyright 2006, 2007, 2008, 2009, 2025 Juan Rey Saura

This file is part of RELB Easy Load Balancer.

This software is a copyrighted work licensed under the Open Software License version 3.0
Please consult the file "LICENSE.txt" for details.
*/

#ifndef utiles_h
#define utiles_h

//#define DEBUG

#include <pinet.h>
#include <ptime.h>

#include <stdio.h>


#ifdef WIN32
#  ifndef FD_SETSIZE
#    error  "FD_SETSIZE must be defined"
#  else 
#    if FD_SETSIZE != 256
#      error "FD_SETSIZE must be 256" // check `pinet.h` in ptypes
#    endif // FD_SETSIZE != 256
#  endif // ! FD_SETSIZE
 //#  include <winsock2.h> // already included in pinet.h once to increase FD_SETSIZE
#else
#  include <sys/time.h>
#  include <sys/types.h>
#  include <sys/socket.h>
#  include <netinet/in.h>
#  include <arpa/inet.h>
#  include <netdb.h>
#  include <unistd.h>
#  include <signal.h>
#  include <time.h>
#endif


USING_PTYPES

#ifndef MAX
#define MAX(x,y) (x) > (y) ? (x) : (y)
#endif

#ifndef MIN
#define MIN(x,y) (x) < (y) ? (x) : (y)
#endif

const char * curr_local_time();
const char * curr_utc_time();
const char * given_local_time( datetime t_utc );
const char * given_utc_time( datetime t );
const char * statusdesc( int status );
ipaddress chartoipaddress( const char * ip );
bool ipmenor( ipaddress * iplt, ipaddress * ipgt );
ipaddress masked_ip( const ipaddress * ip, const ipaddress * mask );
bool checkIP( const ipaddress * ip );
bool checkPort( const ipaddress * ip, unsigned short port );

struct filterinfo
{
  ipaddress src_ip;
  ipaddress src_mask;
  ipaddress dst_ip;
  ipaddress dst_mask;
  bool allow;
};

struct serverinfo
{
  char * host_name;
  ipaddress ip;
  int port;
  int disconnected;
  int finished;
  int connected;
  int connecting;
  int weight;
  int max_connections;
  bool last_connection_failed;
  datetime last_connection_attempt;
};

struct bind_address
{
  ipaddress src_ip;
  unsigned short src_port;
};

#define TASK_TYPE int

#define INVALID_TASK	0
#define TASK_UPDATE_SERVER_INFO	1
#define TASK_CLEAN_CONNECTIONS	2
#define TASK_PURGE_CONNECTIONS	3

#define TASK_UPDATE_SERVER_INFO_TAG	"update"
#define TASK_CLEAN_CONNECTIONS_TAG	"clean"
#define TASK_PURGE_CONNECTIONS_TAG	"purge"

struct task_info
{
  TASK_TYPE task_type;
  int run_interval_seconds;
  datetime next_run_time;
  bool fixed_time;
  datetime last_ran_time;
  int last_ran_exitcode;
};

#define NOW_UTC now()
#define NOW_LOCALTIME  now(false)

#define STATUS_SERVER                     0
#define STATUS_CLIENT                     1 << 0
#define STATUS_CLIENT_SERVER_MASK         (~(STATUS_SERVER|STATUS_CLIENT))
#define STATUS_CONNECTING                 1 << 1
#define STATUS_CONNECTING_SERVER          ((STATUS_CONNECTING)|(STATUS_SERVER))
#define STATUS_CONNECTING_CLIENT          ((STATUS_CONNECTING)|(STATUS_CLIENT))
#define STATUS_CONNECTION_ESTABLISHED     1 << 2
#define STATUS_DISCONNECTED_OK            1 << 3 
#define STATUS_SERVER_DISCONNECTED_OK     ((STATUS_DISCONNECTED_OK)|(STATUS_SERVER))
#define STATUS_CLIENT_DISCONNECTED_OK     ((STATUS_DISCONNECTED_OK)|(STATUS_CLIENT))
#define STATUS_CONNECTION_LOST            1 << 4
#define STATUS_SERVER_CONNECTION_LOST     ((STATUS_CONNECTION_LOST)|(STATUS_SERVER))
#define STATUS_CLIENT_CONNECTION_LOST     ((STATUS_CONNECTION_LOST)|(STATUS_CLIENT))
#define STATUS_CONNECTION_FAILED          1 << 5
#define STATUS_PEER_CONNECTION_TERMINATED 1 << 6
#define STATUS_PEER_CONNECTION_CANCELLED  ((STATUS_PEER_CONNECTION_TERMINATED)|(STATUS_CLIENT))
#define STATUS_PEER_CONNECTION_KICKED     ((STATUS_PEER_CONNECTION_TERMINATED)|(STATUS_SERVER))
#define STATUS_PEER_CONNECTION_BANNED     1 << 7
#define STATUS_PEER_CONNECTION_DELETED    STATUS_PEER_CONNECTION_BANNED
#define STATUS_PEER_CONNECTION_BAN_THIS   ((STATUS_PEER_CONNECTION_BANNED)|(STATUS_SERVER))
#define STATUS_PEER_CONNECTION_BAN_ALL    ((STATUS_PEER_CONNECTION_BANNED)|(STATUS_CLIENT))
#define STATUS_PEER_DISCONNECTED		  ((STATUS_CONNECTION_FAILED)|(STATUS_CONNECTION_LOST))
#define STATUS_PEER_NOT_CONNECTED		  ((STATUS_PEER_DISCONNECTED)|(STATUS_DISCONNECTED_OK))


#define TRACE_IOSOCKET      0
#define TRACE_IOSOCKETERROR 0
#define TRACE_CONNECTIONS   0
#define TRACE_BIND			    0
#define TRACE_ASSIGNATION   0
#define TRACE_CONFIG        1
#define TRACE_TASKS         1
#define TRACE_FILTERS       0
#define TRACE_UNCATEGORIZED 0

#define TRACE_VERBOSE       0
#define TRACE_VERY_VERBOSE  0


#ifdef WIN32
#define socket_errno WSAGetLastError()
#else
#define socket_errno errno
#endif

#ifdef DEBUG
#define TRACE(x) (!x)?(0):printf
#else
#define TRACE(x) (void) //(void)
#endif


#endif
