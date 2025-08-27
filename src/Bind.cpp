/*
   Bind.cpp: bind class source file.

   Copyright 2006, 2007, 2008, 2009 Juan Rey Saura

This file is part of RELB Easy Load Balancer.

This software is a copyrighted work licensed under the Open Software License version 3.0
Please consult the file "LICENSE.txt" for details.
*/

#include "Bind.h"
#include "Socket.h"

#include <stdio.h>

#include "utiles.h"

USING_PTYPES

#ifdef DEBUG
#define BIND_TIMEOUT_MS 6000
#else
#define BIND_TIMEOUT_MS 6000
#endif

Bind::Bind( int peers_per_thread ): thread( false )
{
  finish = false;
  adminEnabled = false;
  if( peers_per_thread > 0 )
    slist.setPeersPerThread( peers_per_thread );
}

void Bind::addAddress( unsigned short listeningPort, ipaddress listeningIP )
{
  bind_address * tmp = new bind_address;
  tmp->src_ip = listeningIP;
  tmp->src_port = listeningPort;
  addresses.add( tmp );
}

void Bind::setServerRetry( int seconds )
{
  slist.setServerRetry( seconds );
}

void Bind::setAdmin( bool enable, const ipaddress server_ip, unsigned short server_port )
{
  adminEnabled = enable;
  adminIP = server_ip;
  adminPort = server_port;
}


Bind::~Bind()
{
  finish = true;
  waitfor();
  while( addresses.get_count() )
  {
    addresses.del( 0 );
  }
}

bool Bind::addServer( const char * nombre, const ipaddress server_ip, unsigned short server_port, int weight, int max_connections )
{
  slist.addServer( nombre, &server_ip, server_port, weight, max_connections );
  admin.addServer( nombre, &server_ip, server_port, weight, max_connections );

  return true;
}

bool Bind::addTask( TASK_TYPE type, datetime firstrun, int run_interval_ms )
{
  return slist.addTask( type, firstrun, run_interval_ms );
}

bool Bind::addTask( TASK_TYPE type, int run_interval_ms )
{
  return slist.addTask( type, run_interval_ms );
}

bool Bind::addFilter( const ipaddress source_ip, const ipaddress source_mask, const ipaddress dest_ip, const ipaddress dest_mask, bool allow )
{
  return slist.addFilter( source_ip, source_mask, dest_ip, dest_mask, allow );
}

void Bind::execute()
{
  int i = 0;
  int * socket_array = new int[addresses.get_count()];
  while( i < addresses.get_count() )
  {
    // set up sockaddr_in and try to bind it to the socket
    sockaddr_in sa;
    memset( &sa, 0, sizeof( sa ) );
    sa.sin_family = AF_INET;
    sa.sin_port = htons( u_short( addresses[i]->src_port ) );
    sa.sin_addr.s_addr = inet_addr( iptostring( addresses[i]->src_ip ) );

    int sockfd;
    sockfd = socket( sa.sin_family, SOCK_STREAM, 0 );

    if( sockfd < 0 )
    {
      TRACE( TRACE_CONNECTIONS )( "%s - binding socket not created\n", curr_local_time() );
    }

#ifndef WIN32
    // Acording to Apache code: on windows this option causes
    // the previous owner of the socket to give up, which is not desirable
    // in most cases, neither compatible with unix.
    int reuse = 1;
    if( ::setsockopt( sockfd, SOL_SOCKET, SO_REUSEADDR, (const void *) &reuse, sizeof( reuse ) ) != 0 )
    {
      TRACE( TRACE_CONNECTIONS )( "%s - socket address was not reusable\n", curr_local_time() );
    }
#endif

    if( bind( sockfd, (sockaddr *) &sa, sizeof( sa ) ) != 0 )
    {
      TRACE( TRACE_CONNECTIONS )( "%s - socket could not be binded\n", curr_local_time() );
    }

    if( listen( sockfd, 100 ) != 0 )
    {
      TRACE( TRACE_CONNECTIONS )( "%s - could not stat listening\n", curr_local_time() );
    }


    socket_array[i] = sockfd;
    i++;
  }

  fd_set set;
  int timeout = BIND_TIMEOUT_MS;
  timeval t, to;
  to.tv_sec = timeout / 1000;
  to.tv_usec = ( timeout % 1000 ) * 1000;

  if( adminEnabled )
  {
    admin.setParallelList( slist.getQueue() );
    admin.startHTTPAdmin( adminPort, adminIP );
    slist.setParallelList( admin.getJQ() );
  }

  while( !finish )
  {
    //int ret;
    FD_ZERO( &set );
    for( i = 0; i < addresses.get_count(); i++ )
    {
      FD_SET( socket_array[i], &set );
    }
    t.tv_sec = to.tv_sec;
    t.tv_usec = to.tv_usec;

    TRACE( TRACE_CONNECTIONS )( "%s - waiting connection\n", curr_local_time() );

    if( /*ret = */::select( FD_SETSIZE, &set, nil, nil, /*(timeout < 0) ? nil : */&t ) > 0 )
    {
      TRACE( TRACE_CONNECTIONS )( "%s - new connection\n", curr_local_time() );
      sockaddr_in sac;
      memset( &sac, 0, sizeof( sac ) );
#ifdef WIN32
      int addrlen = sizeof( sac );
#else
      socklen_t addrlen = sizeof( sac );
#endif

      for( i = 0; i < addresses.get_count(); i++ )
      {
        if( FD_ISSET( socket_array[i], &set ) )
        {
          TRACE( TRACE_CONNECTIONS )( "%s - accepting connection\n", curr_local_time() );
          int cliente = accept( socket_array[i], (sockaddr *) &sac, &addrlen );
          TRACE( TRACE_CONNECTIONS )( "%s - accepted connection\n", curr_local_time() );
          if( cliente > 0 )
          {
            slist.setServer( cliente, &sac );
          }
        }
      }

    }
  }

  admin.stopHTTPAdmin();

  for( i = 0; i < addresses.get_count(); i++ )
  {
    ::closesocket( socket_array[i] );
  }

  if( socket_array )
    delete [] socket_array;
}

void Bind::cleanup()
{
}

int Bind::startListening()
{
  if( addresses.get_count() > 0 )
  {
    slist.start();
    start();
  }
  return 0;
}

int Bind::stopListening()
{
  finish = true;
  waitfor();
  return 0;
}


