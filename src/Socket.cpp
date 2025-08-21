/*
   Socket.cpp: Cross-platform socket wrapper class implementation

   Provides non-blocking socket operations with platform-specific optimizations
   for Windows and Unix-like systems.

   Copyright 2006, 2007, 2008, 2009 Juan Rey Saura

This file is part of RELB Easy Load Balancer.

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

// Set non-blocking flags based on platform support
#ifndef MSG_DONTWAIT
#define NONBLOCKINGFLAGS 0  // Default to blocking mode if MSG_DONTWAIT not available
#else
#define NONBLOCKINGFLAGS MSG_DONTWAIT
#endif

USING_PTYPES

// Constructor for accepted connections
// @param sockfd: Existing socket file descriptor
// @param srcip: Source IP address
// @param srcport: Source port number
Socket::Socket( int sockfd, ipaddress * srcip, unsigned short srcport )
{
  sock = sockfd;
  port = srcport;
  ip = *( srcip );

#ifdef WIN32 
  // Set non-blocking mode for Windows sockets
  int nonBlocking = 1;
  ioctlsocket( sock, FIONBIO, ( u_long FAR * ) & nonBlocking );
#endif

  TRACE( TRACE_CONNECTIONS )( "%s - Created connection %d port %d from ip address %s\n", curr_local_time(), sock, port, (const char *) iptostring( *srcip ) );
}

// Constructor for outbound connections
// @param dstip: Destination IP address
// @param dstport: Destination port number
Socket::Socket( const ipaddress * dstip, unsigned short dstport )
{
  ip = *dstip;
  port = dstport;

  // Create TCP socket
  sock = socket( AF_INET, SOCK_STREAM, 0 );
  TRACE( TRACE_CONNECTIONS )( "%s - Creating the socket %d\n", curr_local_time(), sock );
  TRACE( TRACE_CONNECTIONS )( "%s - Trying to connect to port %d IP %s\n", curr_local_time(), port, (const char *) iptostring( ip ) );

  if( sock < 0 )
  {
    TRACE( TRACE_CONNECTIONS )( "%s - FAILED to create socket\n", curr_local_time() );
  }
  else
  {
    TRACE( TRACE_CONNECTIONS )( "%s - socket created properly\n", curr_local_time() );
  }

#ifdef WIN32
  // Set non-blocking mode for Windows sockets
  long nonBlocking = 1;
  ioctlsocket( sock, FIONBIO, ( u_long FAR * ) & nonBlocking );
#endif

  // Initialize and connect socket
  sockaddr_in sa;
  memset( &sa, 0, sizeof( sa ) );
  sa.sin_family = AF_INET;
  sa.sin_port = htons( port );
  sa.sin_addr.s_addr = ip;
  connect( sock, (sockaddr *) &sa, sizeof( sa ) );
  TRACE( TRACE_CONNECTIONS )( "%s - exiting from connect()\n", curr_local_time() );
}

// Constructor using hostname instead of IP
// @param dsthost: Destination hostname
// @param dstport: Destination port number
Socket::Socket( const char * dsthost, int dstport )
{
  ip = chartoipaddress( dsthost );
  Socket( &ip, dstport );
}

// Destructor - ensures socket is closed
Socket::~Socket()
{
  close();
}

// Placeholder for any cleanup needed
void Socket::cleanup()
{
}

// Add this socket to an fd_set for select() monitoring
// @param set: fd_set to add this socket to
void Socket::addToFDSET( fd_set * set )
{
  //  if( sock > 0 )
  FD_SET( sock, set );

#ifdef DEBUG
  // Debug trace for socket state
  if( sock < 0 )
    TRACE( TRACE_IOSOCKET )( "%s - SOCKET ALREADY CORRUPT IN FDSET %d\n", curr_local_time(), sock );
  else
    TRACE( TRACE_IOSOCKET && TRACE_VERY_VERBOSE )( "%s - SOCKET IN FDSET %d\n", curr_local_time(), sock );
#endif
}

// Check if socket is set in an fd_set
// @param set: fd_set to check
// @return: True if socket is in set and valid
int Socket::isInFDSET( fd_set * set )
{
  return ( sock > 0 ) ? FD_ISSET( sock, set ) : 0;
}

// Non-blocking send operation
// @param buffer: Data to send
// @param max: Maximum bytes to send
// @return: >0: bytes sent, 0: would block, -1: error, -2: peer closed
int Socket::sendNB( const char * buffer, int max )
{
  int ret = -1;

  if( sock > 0 )
  {
    ret = send( sock, buffer, max, NONBLOCKINGFLAGS );
    TRACE( TRACE_IOSOCKET && TRACE_VERY_VERBOSE )( "%s - could wrote %d\n", curr_local_time(), ret );
    if( ret < 0 )
    {
      if( ( socket_errno == EAGAIN ) || ( socket_errno == EWOULDBLOCK ) )
      {
        // Would block - try again later
        ret = 0;
      }
      else // ECONNRESET Connection reset or other errors
      {

        TRACE( TRACE_IOSOCKET )( "%s - I could not write all in socket\n", curr_local_time() );
#ifdef WIN32
        TRACE( TRACE_IOSOCKET )( "%s - Error %d %d\n", curr_local_time(), socket_errno, WSAGetLastError() );
#endif
        close();
      }
    }
    else
    {
      if( ( ret == 0 ) )
      {
        // Peer closed connection
        TRACE( TRACE_CONNECTIONS )( "%s - peer closed properly, detected on write\n", curr_local_time() );
        close();
        ret = -2;
      }
    }
  }

  return ret;
}

// Non-blocking receive operation
// @param buffer: Buffer to receive into
// @param max: Maximum bytes to receive
// @return: >0: bytes received, 0: would block, -1: error, -2: peer closed
int Socket::recieveNB( char * buffer, int max )
{
  int ret = -1;

  if( sock > 0 )
  {
    ret = recv( sock, buffer, max, NONBLOCKINGFLAGS );
    TRACE( TRACE_IOSOCKET && TRACE_VERY_VERBOSE )( "%s - could read %d\n", curr_local_time(), ret );
    if( ret < 0 )
    {
      if( ( socket_errno == EAGAIN ) || ( socket_errno == EWOULDBLOCK ) )
      {
        // Would block - try again later
        TRACE( TRACE_IOSOCKETERROR )( "%s - I could not read all from socket EWOULDBLOCK \n", curr_local_time() );
        ret = 0;
      }
      else
      {
        TRACE( TRACE_IOSOCKETERROR )( "%s -I could not read all from socket\n", curr_local_time() );
#ifdef WIN32
        TRACE( TRACE_IOSOCKET )( "%s - Error %d %d\n", curr_local_time(), errno, WSAGetLastError() );
#endif
        close();

      }
    }
    else
    {
      if( ret == 0 )
      {
        // Peer closed connection
        TRACE( TRACE_CONNECTIONS )( "%s - peer closed properly, detected on read\n", curr_local_time() );
        close();
        ret = -2;
      }

    }
  }

  return ret;
}

// Close the socket and mark as invalid
void Socket::close()
{
  if( sock > 0 )
  {
    TRACE( TRACE_IOSOCKET )( "%s - Closing socket\n", curr_local_time() );
    ::closesocket( sock );
    sock = -1;
  }
}

#ifdef DEBUG
// Debug version of close() with different trace message
void Socket::close2()
{
  if( sock > 0 )
  {
    TRACE( TRACE_IOSOCKET )( "%s - closing DEBUG socket\n", curr_local_time() );
    ::closesocket( sock );
  }
}
#endif

// Placeholder for socket retrieval
void Socket::getSocket()
{
}

// Check socket for readability using select()
// @return: >0: readable, 0: not readable, -1: error
int Socket::checkSocket()
{
  int ret = -1;

  TRACE( TRACE_IOSOCKET && TRACE_VERY_VERBOSE )( "%s - I am going to check the socket %d\n", curr_local_time(), sock );

  if( sock > 0 )
  {
    // Set up select() with zero timeout for immediate check
    fd_set setr;
    timeval to;
    to.tv_sec = 0;
    to.tv_usec = 0;
    FD_ZERO( &setr );
    FD_SET( sock, &setr );
    ret = ::select( FD_SETSIZE, &setr, nil, nil, &to );
    TRACE( TRACE_IOSOCKET && TRACE_VERY_VERBOSE )( "%s - Really checking the socket, ret was %d but not %d\n", curr_local_time(), ret, int( -1 ) );
  }

  return ret;
}
