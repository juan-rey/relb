/*
   MessaggeQueue.cpp: MessaggeQueue class source file.

   Copyright 2006, 2007, 2008, 2009 Juan Rey Saura

This file is part of RELB Easy Load Balancer.

This software is a copyrighted work licensed under the Open Software License version 3.0
Please consult the file "LICENSE.txt" for details.
*/

#ifndef WIN32
#include <unistd.h>
#endif

#include "ControlObject.h"
#include "utiles.h"

USING_PTYPES


#ifndef MSG_DONTWAIT
#define NONBLOCKINGFLAGS 0
#else
#define NONBLOCKINGFLAGS MSG_DONTWAIT
#endif

ControlObject::ControlObject( void )
{
}

ControlObject::~ControlObject( void )
{
}

ControlSocket::ControlSocket( void )
{
#ifdef WIN32
  sock = socket( AF_INET, SOCK_STREAM, 0 );
  TRACE( TRACE_CONNECTIONS )( "%s - creating controlsocket %d\n", curr_local_time(), sock );
#else
  fd[0] = fd[1] = -1;
  if( pipe( fd ) == -1 )
  {
  }
#endif

}

ControlSocket::~ControlSocket( void )
{
#ifdef WIN32	
  closesocket( sock );
#else
  closesocket( fd[0] );
  closesocket( fd[1] );
#endif	
}

void ControlSocket::post()
{
#ifdef WIN32	
  int oldsock = sock;
  int tmpsock = -1;

  tmpsock = socket( AF_INET, SOCK_STREAM, 0 );

  TRACE( TRACE_CONNECTIONS )( "%s - i create control socket %d\n", curr_local_time(), tmpsock );
  if( tmpsock == oldsock )
  {
    tmpsock = socket( AF_INET, SOCK_STREAM, 0 );
    closesocket( oldsock );
    TRACE( TRACE_CONNECTIONS )( "%s - i have create control socket again, %d, old socked fd was identicall to old one\n", curr_local_time(), tmpsock );
  }


  sock = tmpsock;
  TRACE( TRACE_CONNECTIONS )( "%s - i have closed old control socket %d\n", curr_local_time(), oldsock );

  if( closesocket( oldsock ) != 0 )
  {
    //closesocket( oldsock );
    TRACE( TRACE_CONNECTIONS )( "%s - Could NOT old control socket %d\n", curr_local_time(), oldsock );
  }
#else
  int ret = write( fd[1], buffer, 1 );//, NONBLOCKINGFLAGS );
  TRACE( TRACE_VERY_VERBOSE )( "%s - to control socket %d wrote %d\n", curr_local_time(), fd[0], ret );
#endif		
}

void ControlSocket::addToFDSET( fd_set * set )
{
#ifdef WIN32		
  if( sock > 0 )
  {
    TRACE( TRACE_VERY_VERBOSE )( "%s - i am adding to FDSET control socket %d\n", curr_local_time(), sock );
    FD_SET( sock, set );
  }
#else
  if( fd[0] > 0 )
  {
    TRACE( TRACE_VERY_VERBOSE )( "%s - i am adding to FDSET control socket %d\n", curr_local_time(), fd[0] );
    FD_SET( fd[0], set );
  }
#endif
}

#ifdef WIN32	
int ControlSocket::checkSocket()
#else
int ControlSocket::checkSocket( fd_set * set )
#endif
{
  int ret = -1;
#ifdef WIN32		
  int tmpsock = sock;

  TRACE( TRACE_VERY_VERBOSE )( "%s - i am checking control socket %d\n", curr_local_time(), tmpsock );

  if( tmpsock > 0 )
  {
    fd_set setr;
    timeval to;
    to.tv_sec = 0;
    to.tv_usec = 0;
    FD_ZERO( &setr );
    FD_SET( tmpsock, &setr );
    ret = ::select( FD_SETSIZE, &setr, nil, nil, &to );
    TRACE( TRACE_VERY_VERBOSE )( "%s - select control socket %d returned %d\n", curr_local_time(), tmpsock, ret );
  }

  if( ret < 0 )
  {
    post();
  }
#else
  ret = 0;
  if( FD_ISSET( fd[0], set ) )
  {
    ret = read( fd[0], buffer, 1 );//, NONBLOCKINGFLAGS )) > 0 )
    TRACE( TRACE_VERY_VERBOSE )( "%s - control socket %d read %d\n", curr_local_time(), fd[0], ret );
  }
#endif	

  return ret;
}
