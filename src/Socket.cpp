/* 
   Socket.cpp: socket class source file.

   Copyright 2006, 2007, 2008, 2009 Juan Rey Saura

This file is part of Resolutive Easy Load Balancer.

This software is a copyrighted work licensed under the Open Software License version 3.0
Please consult the file "LICENSE.txt" for details.
*/

#include "Socket.h"
#include "ControlObject.h"

#include <pinet.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdio.h>

#include "utiles.h"

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

#ifndef MSG_DONTWAIT
#define NONBLOCKINGFLAGS 0
#else
#define NONBLOCKINGFLAGS MSG_DONTWAIT
#endif

USING_PTYPES

Socket::Socket( int sockfd, ipaddress * srcip, unsigned short srcport )
{
  sock = sockfd;
  port = srcport;
  ip = *(srcip);

#ifdef WIN32 
 int nonBlocking = 1;
 ioctlsocket( sock, FIONBIO, (u_long FAR*) &nonBlocking); 
#endif

// already_connected = true;
 TRACE( TRACE_CONNECTIONS )( "%s - Created connection %d port %d from ip address %s\n", curr_local_time(), sock, port, (const char*)iptostring(*srcip));
}

Socket::Socket( const ipaddress * dstip, unsigned short dstport )//, ControlObject * notify, int timeout )
{
  ip = *dstip; 
  port = dstport;
  sock = socket( AF_INET, SOCK_STREAM, 0 );
  TRACE( TRACE_CONNECTIONS )( "%s - Creating the socket %d\n",  curr_local_time(), sock ); 
  TRACE( TRACE_CONNECTIONS )( "%s - Trying to connect to port %d IP %s\n",  curr_local_time(), port, (const char*)iptostring( ip )); 
  
  if( sock < 0 )
  {
    TRACE( TRACE_CONNECTIONS )( "%s - FAILED to create socket\n",  curr_local_time() );
  }
  else
  {
    TRACE( TRACE_CONNECTIONS )( "%s - socket created properly\n",  curr_local_time() );    
  }
    
#ifdef WIN32
 long nonBlocking = 1;
 ioctlsocket( sock, FIONBIO, (u_long FAR*) &nonBlocking);
#endif

	sockaddr_in sa;
	memset(&sa, 0, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_port = htons( port );
	sa.sin_addr.s_addr = ip;  
	connect( sock, (sockaddr*)&sa, sizeof(sa));
	TRACE( TRACE_CONNECTIONS )("%s - exiting from connect()\n", curr_local_time());  
}

Socket::Socket( const char * dsthost, int dstport)
{
  ip = chartoipaddress(dsthost);
  Socket( &ip, dstport ); 
}

Socket::~Socket()
{
  close();
}


void Socket::cleanup()
{
}

void Socket::addToFDSET( fd_set * set )
{
//  if( sock > 0 )
    FD_SET( sock, set );

#ifdef DEBUG
if( sock < 0 )
    TRACE( TRACE_IOSOCKET )("%s - SOCKET ALREADY CORRUPT IN FDSET %d\n", curr_local_time(), sock );
else
    TRACE( TRACE_VERY_VERBOSE )("%s - SOCKET IN FDSET %d\n", curr_local_time(), sock );
#endif

}

int Socket::isInFDSET( fd_set * set )
{
  return ( sock > 0 )?FD_ISSET( sock, set ):0;
}

int Socket::sendNB( const char * buffer, int max )
{
  int ret  = -1; 
  
  if( sock > 0 )
  { 
    ret = send( sock,  buffer, max, NONBLOCKINGFLAGS );
	TRACE( (TRACE_VERY_VERBOSE &&  TRACE_IOSOCKET) )("%s - could wrote %d\n", curr_local_time(), ret );
    if( ret < 0 )
    {
      if(( socket_errno == EAGAIN ) || ( socket_errno == EWOULDBLOCK ))
      {
        ret = 0;
      }
      else //otros errores como ECONNRESET
      {
	   
       TRACE( TRACE_IOSOCKET )("%s - I coud not write all in socket\n", curr_local_time());
#ifdef WIN32
       TRACE( TRACE_IOSOCKET )("%s - Error %d %d\n", curr_local_time(), socket_errno, WSAGetLastError() );
#endif
       close();
      }    
    }
    else
    {
      if( ( ret == 0 ) )
      {
        TRACE( TRACE_CONNECTIONS )("%s - peer closed properly, detected on write\n", curr_local_time());;
        close();
        ret = -2;
      }
    }
  }
  
  return ret;  
}

int Socket::recieveNB( char * buffer,  int max )
{
  int ret  = -1;
  
  if( sock > 0 )
  {
    ret = recv( sock,  buffer, max, NONBLOCKINGFLAGS );
	TRACE( (TRACE_VERY_VERBOSE && TRACE_IOSOCKET) )("%s - could read %d\n", curr_local_time(), ret );
    if( ret < 0 )
    {
      if(( socket_errno == EAGAIN)||( socket_errno == EWOULDBLOCK ))
      {
       TRACE( TRACE_IOSOCKETERROR )("%s - I could not read all from socket EWOULDBLOCK \n", curr_local_time());		  
        ret = 0;
      }
      else
      {
       TRACE( TRACE_IOSOCKETERROR )("%s -I could not read all from socket\n", curr_local_time());
#ifdef WIN32
       TRACE( TRACE_IOSOCKET )("%s - Error %d %d\n", curr_local_time(), errno, WSAGetLastError() );
#endif
       close();

      }
    }
    else
    {
      if( ret == 0 )
      {
        TRACE( TRACE_CONNECTIONS )("%s - peer closed properly, detected on read\n", curr_local_time());
        close();
        ret = -2;
      }

    }
  }
  
  return ret;    
}

void Socket::close()
{
  if( sock > 0 )
  {
    TRACE( TRACE_IOSOCKET )("%s - Closing socket\n", curr_local_time());   
    ::closesocket(sock);
    sock = -1;
  }
}

#ifdef DEBUG
void Socket::close2()
{
  if( sock > 0 )
  {
    TRACE( TRACE_IOSOCKET )("%s - closing DEBUG socket\n", curr_local_time());   
    ::closesocket(sock);
  }
}
#endif

void Socket::getSocket()
{
}

int Socket::checkSocket()
{
	int ret = -1;
	
	TRACE( TRACE_VERY_VERBOSE )("%s - I am going to check the socket %d\n", curr_local_time(), sock ); 

	if( sock > 0 )
	{
		fd_set setr;
		timeval to;
		to.tv_sec = 0;
		to.tv_usec = 0;
		FD_ZERO( &setr );
		FD_SET( sock, &setr );
		ret =::select(FD_SETSIZE, &setr, nil, nil, &to );
		TRACE( TRACE_VERY_VERBOSE )("%s - Really checking the socket, ret was %d but not %d\n", curr_local_time(), ret, int(-1));
	}

	return ret;
}
