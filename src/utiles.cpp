/*
   utiles.cpp: utiles source file.

   Copyright 2006, 2007, 2008, 2009, 2025 Juan Rey Saura

This file is part of RELB Easy Load Balancer.

This software is a copyrighted work licensed under the Open Software License version 3.0
Please consult the file "LICENSE.txt" for details.
*/

#include <pasync.h>    // for psleep()
#include <ptime.h>
#include <pstreams.h>

#include <stdio.h>
#include <stdlib.h>

#ifdef WIN32
#  include <winsock2.h>
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

#include "utiles.h"

USING_PTYPES

char currutctimestring[50] = "";

const char * curr_utc_time()
{
#ifdef DEBUG    
  int hours, mins, secs, msecs;
  int year, month, day;
  decodedate( NOW_UTC, year, month, day );
  decodetime( NOW_UTC, hours, mins, secs, msecs );
  sprintf( currutctimestring, "%d-%02d-%02d %02d:%02d:%02d,%03d", year, month, day, hours, mins, secs, msecs );
#endif    
  return currutctimestring;
}


char currlocaltimestring[50] = "";

const char * curr_local_time()
{
#ifdef DEBUG    
  int hours, mins, secs, msecs;
  int year, month, day;
  decodedate( NOW_LOCALTIME, year, month, day );
  decodetime( NOW_LOCALTIME, hours, mins, secs, msecs );
  sprintf( currlocaltimestring, "%d-%02d-%02d %02d:%02d:%02d,%03d", year, month, day, hours, mins, secs, msecs );
#endif    
  return currlocaltimestring;
}


char givenlocaltimestring[50] = "";

const char * given_local_time( datetime t_utc )
{
#ifdef DEBUG    
  int hours, mins, secs, msecs;
  int year, month, day;
  t_utc = t_utc + ( NOW_LOCALTIME - NOW_UTC );
  decodedate( t_utc, year, month, day );
  decodetime( t_utc, hours, mins, secs, msecs );
  sprintf( givenlocaltimestring, "%d-%02d-%02d %02d:%02d:%02d,%03d", year, month, day, hours, mins, secs, msecs );
#endif    
  return givenlocaltimestring;
}


char givenutctimestring[50] = "";

const char * given_utc_time( datetime t )
{
#ifdef DEBUG    
  int hours, mins, secs, msecs;
  int year, month, day;
  decodedate( t, year, month, day );
  decodetime( t, hours, mins, secs, msecs );
  sprintf( givenutctimestring, "%d-%02d-%02d %02d:%02d:%02d,%03d", year, month, day, hours, mins, secs, msecs );
#endif    
  return givenutctimestring;
}

const char * statusdesc( int status )
{
  const char * ret = "";
  switch( status & STATUS_CLIENT_SERVER_MASK )
  {
    case STATUS_CONNECTING:
      ret = "connecting";
      break;
    case STATUS_CONNECTION_ESTABLISHED:
      ret = "connected";
      break;
    case STATUS_CONNECTION_FAILED:
      ret = "refused";
      break;
    case STATUS_CONNECTION_LOST:
      ret = "connection lost";
      break;
    case STATUS_DISCONNECTED_OK:
      ret = "finished";
      break;
  }
  return ret;
}

#define IPADDRESS_SIZE 4   

bool ipmenor( ipaddress * iplt, ipaddress * ipgt )
{
  bool menor = false;
  for( int i = 0; i < IPADDRESS_SIZE; i++ )
  {
    if( iplt->data[i] > ipgt->data[i] )
    {
      i = IPADDRESS_SIZE;
    }
    else
    {
      if( iplt->data[i] < ipgt->data[i] )
      {
        menor = true;
        i = IPADDRESS_SIZE;

      }
    }
  }

  return menor;
}

ipaddress chartoipaddress( const char * ip )
{
  ipaddress ret = ipnone;
  if( ip != NULL )
  {
    int m = 0;
    int i[IPADDRESS_SIZE];
    int t = 0;

    while( t < IPADDRESS_SIZE )
    {
      i[t] = 0;
      t++;
    }

    t = 0;
    i[t] = atoi( &ip[m] );
    t++;

    while( ( ip[m] != char( NULL ) ) && ( t < IPADDRESS_SIZE ) )
    {
      if( ip[m] == '.' )
      {
        i[t] = atoi( &ip[m + 1] );
        t++;
      }
      m++;
    }

    ret = ipaddress( i[0], i[1], i[2], i[3] );
  }

  return ret;
}


ipaddress masked_ip( const ipaddress * ip, const ipaddress * mask )
{

  return ipaddress( ip->data[0] & mask->data[0], ip->data[1] & mask->data[1], ip->data[2] & mask->data[2], ip->data[3] & mask->data[3] );
}

bool checkIP( const ipaddress * ip )
{
  bool val = true;

  sockaddr_in sa;
  memset( &sa, 0, sizeof( sa ) );
  sa.sin_family = AF_INET;
  sa.sin_port = htons( 0 );
  //sa.sin_port = htons( u_short(port) );
  //sa.sin_addr.s_addr = htonl(INADDR_ANY);
  sa.sin_addr.s_addr = inet_addr( iptostring( *ip ) );

  int sockfd;
  sockfd = socket( sa.sin_family, SOCK_STREAM, 0 );

  if( sockfd < 0 )
  {
    TRACE( TRACE_BIND && TRACE_VERY_VERBOSE )( "%s - could not create socket on that ip\n", curr_local_time() );
    val = false;
  }

#ifndef WIN32
  // Acording to Apache code: on windows this option causes
  // the previous owner of the socket to give up, which is not desirable
  // in most cases, neither compatible with unix.
  int reuse = 1;
  if( ::setsockopt( sockfd, SOL_SOCKET, SO_REUSEADDR, (const void *) &reuse, sizeof( reuse ) ) != 0 )
  {
    TRACE( TRACE_BIND && TRACE_VERY_VERBOSE )( "%s - socket address was not reusable\n", curr_local_time() );
    val = false;
  }
#endif

  if( bind( sockfd, (sockaddr *) &sa, sizeof( sa ) ) != 0 )
  {
    TRACE( TRACE_BIND && TRACE_VERY_VERBOSE )( "%s - socket could not be binded\n", curr_local_time() );
    val = false;
  }

  //¿ Puedo borrar el listen ?
  if( listen( sockfd, 10 ) != 0 )
  {
    TRACE( TRACE_BIND && TRACE_VERY_VERBOSE )( "%s - could not start listening\n", curr_local_time() );
    val = false;
  }

  closesocket( sockfd );

  return val;
}

bool checkPort( const ipaddress * ip, unsigned short port )
{
  bool val = true;

  sockaddr_in sa;
  memset( &sa, 0, sizeof( sa ) );
  sa.sin_family = AF_INET;
  sa.sin_port = htons( 0 );
  sa.sin_port = htons( u_short( port ) );
  sa.sin_addr.s_addr = htonl( INADDR_ANY );
  sa.sin_addr.s_addr = inet_addr( iptostring( *ip ) );

  int sockfd;
  sockfd = socket( sa.sin_family, SOCK_STREAM, 0 );

  if( sockfd < 0 )
    val = false;

#ifndef WIN32
  // Acording to Apache code: on windows this option causes
  // the previous owner of the socket to give up, which is not desirable
  // in most cases, neither compatible with unix.
  int reuse = 1;
  if( ::setsockopt( sockfd, SOL_SOCKET, SO_REUSEADDR, (const void *) &reuse, sizeof( reuse ) ) != 0 )
  {
    TRACE( TRACE_BIND && TRACE_VERY_VERBOSE )( "%s - socket address was not reusable\n", curr_local_time() );
    val = false;
  }
#endif

  if( bind( sockfd, (sockaddr *) &sa, sizeof( sa ) ) != 0 )
  {
    TRACE( TRACE_BIND && TRACE_VERY_VERBOSE )( "%s - socket could not be binded\n", curr_local_time() );
    val = false;
  }

  if( listen( sockfd, 10 ) != 0 )
  {
    TRACE( TRACE_BIND && TRACE_VERY_VERBOSE )( "%s - could not start listening\n", curr_local_time() );
    val = false;
  }

  closesocket( sockfd );

  return val;
}
