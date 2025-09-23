/*
   ThreadedConnectionManager.cpp: connection manager source file.

   Copyright 2006, 2007, 2008, 2009 Juan Rey Saura

This file is part of RELB Easy Load Balancer.

This software is a copyrighted work licensed under the Open Software License version 3.0
Please consult the file "LICENSE.txt" for details.
*/

#include "ThreadedConnectionManager.h"
#include "utiles.h"

USING_PTYPES

ThreadedConnectionManager::ThreadedConnectionManager( int connections ): thread( false )
{
  TRACE( ( TRACE_UNCATEGORIZED && TRACE_VERY_VERBOSE ) )( "%s - New connection manager to handle up to %d connections\n", curr_local_time(), connections );
  max_connections = MIN( connections, MAX_PEERS_PER_FDSET );
  TRACE( ( TRACE_UNCATEGORIZED && TRACE_VERY_VERBOSE ) )( "%s - Actually, up to %d connections\n", curr_local_time(), max_connections );
  currentconnections = 0;
  finish = false;
  start();
}

ThreadedConnectionManager::~ThreadedConnectionManager()
{
  finish = true;
  control_socket.post();
  waitfor();
  cleanup();
}

void ThreadedConnectionManager::execute()
{
  int connected_peers = 0;
  int connecting_peers = 0;
  int i = 0;
  int res = 0;
  fd_set setr;
  fd_set setw;
  int max_fd = 0;

  ConnectionPeer * cpeer = NULL;
#ifdef DEBUG
  int times_check = 1;
#endif

  while( !finish )
  {

    // Purge the list of peers to be closed
    while( to_be_closed.get_count() )
    {
      // Check if it's connected
      i = 0;
      while( i < peer_list.get_count() )
      {
        peer_list[i]->closePInfo( to_be_closed.top() );
        i++;
      }

      // Check if it's connecting
      i = 0;
      while( i < connecting_peer_list.get_count() )
      {
        connecting_peer_list[i]->closePInfo( to_be_closed.top() );
        i++;
      }
      to_be_closed.pop();
    }

    FD_ZERO( &setr );
    FD_ZERO( &setw );
#ifdef ENABLE_SELECT_NFDS_CALC // see comment in utiles.h
    max_fd = 0; // nfds is the highest file descriptor plus one, in Windows is ignored
#else
    max_fd = FD_SETSIZE - 1; // very old ugly trick based on old documentation of select which recommended to use FD_SETSIZE as the first parameter
#endif // ENABLE_SELECT_NFDS_CALC

    // Check the peer_list to add the active peers to the fd_set otherwise they will be removed
    connected_peers = 0;
    while( connected_peers < peer_list.get_count() )
    {
      cpeer = peer_list[connected_peers];
      if( cpeer )
      {
        if( !finish && cpeer->isActive() )
        {
#ifdef ENABLE_SELECT_NFDS_CALC
          cpeer->addToFDSETR( &setr, &max_fd );
          cpeer->addToFDSETW( &setw, &max_fd );
#else
          cpeer->addToFDSETR( &setr );
          cpeer->addToFDSETW( &setw );
#endif // ENABLE_SELECT_NFDS_CALC
        }
        else
        {
          TRACE( ( TRACE_CONNECTIONS && TRACE_VERBOSE ) )( "%s - There is an inactive peer, deleting it\n", curr_local_time() );
          peer_list.del( connected_peers );
          delete cpeer;
          TRACE( ( TRACE_CONNECTIONS && TRACE_VERBOSE ) )( "%s - There was an inactive peer, remain %d %d %d \n", curr_local_time(), peer_list.get_count(), connected_peers, connecting_peers );
          connected_peers--;
          currentconnections--;
        }
      }
      connected_peers++;
    }

    // Check the connecting_peer_list to add the active peers to the fd_set otherwise they will be removed
    connecting_peers = 0;
    while( connecting_peers < connecting_peer_list.get_count() )
    {
      TRACE( ( TRACE_CONNECTIONS && TRACE_VERBOSE ) )( "%s - Checking ongoing server connections\n", curr_local_time() );
      cpeer = connecting_peer_list[connecting_peers];
      if( cpeer )
      {
        if( !finish && cpeer->isActive() )
        {
          TRACE( TRACE_IOSOCKETERROR )( "%s - Adding connecting socket to select\n", curr_local_time() );
#ifdef ENABLE_SELECT_NFDS_CALC
          cpeer->addToFDSETC( &setr, &setw, &max_fd );
#else
          cpeer->addToFDSETC( &setr, &setw );
#endif // ENABLE_SELECT_NFDS_CALC
        }
        else
        {
          // TODO LATER 1.1 add disconnect notification here
          TRACE( ( TRACE_CONNECTIONS && TRACE_VERBOSE ) )( "%s - There is an inactive peer, deleting it\n", curr_local_time() );
          connecting_peer_list.del( connecting_peers );
          delete cpeer;
          TRACE( ( TRACE_CONNECTIONS && TRACE_VERBOSE ) )( "%s - There was an inactive peer, remain %d %d %d\n", curr_local_time(), connecting_peer_list.get_count(), connected_peers, connecting_peers );
          connecting_peers--;
          currentconnections--;
        }
      }
      connecting_peers++;
    }

    // Check changes in all active sockets
    if( !finish )
    {
      //TODO check fd_count of setw; otherwise nil  ???
#ifdef ENABLE_SELECT_NFDS_CALC // see comment in utiles.h
      control_socket.addToFDSET( &setr, &max_fd );
      TRACE( ( TRACE_NFDS && 1 ) )( "%s - max_fd=%d\n", curr_local_time(), max_fd );
#else
      control_socket.addToFDSET( &setr );
      TRACE( ( TRACE_NFDS && 1 ) )( "%s - NO NFDS CALC, max_fd=%d\n", curr_local_time(), max_fd );
#endif // ENABLE_SELECT_NFDS_CALC

      if( connected_peers || connecting_peers )
      {
        TRACE( ( TRACE_IOSOCKET && TRACE_VERY_VERBOSE ) )( "%s - Checking socket changes - with some connections\n", curr_local_time() );
        // According to the documentation, the first parameter is the highest file descriptor plus one except in Windows where it is ignored
        res = ::select( max_fd + 1, &setr, &setw, nil, nil ); // timeout is nil, so it will block until there is a change in the sockets
        TRACE( ( TRACE_IOSOCKET && TRACE_VERY_VERBOSE ) )( "%s - Exit - Checking socket changes - with some connections\n", curr_local_time() );
      }
      else
      {
        // No connections, so only check the control socket in setr
        TRACE( ( TRACE_IOSOCKET && TRACE_VERY_VERBOSE ) )( "%s - Checking socket changes - with no connections\n", curr_local_time() );
        // According to the documentation, the first parameter is the highest file descriptor plus one except in Windows where it is ignored
        res = ::select( max_fd + 1, &setr, nil, nil, nil ); // timeout is nil, so it will block until there is a change in the control socket
        TRACE( ( TRACE_IOSOCKET && TRACE_VERY_VERBOSE ) )( "%s - Exit - Checking socket changes - with no connections\n", curr_local_time() );
      }

      if( res > 0 )
      {
#ifndef WIN32          
        control_socket.checkSocket( &setr );
#endif
#ifdef DEBUG           
        if( !( connected_peers || connecting_peers ) )
          TRACE( ( TRACE_IOSOCKET && TRACE_VERBOSE ) )( "%s - Checking socket changes with res %d\n", curr_local_time(), res );
#endif         

        // Use manageConnections to check the changes in the sockets and do the necessary actions
        i = 0;
        TRACE( ( TRACE_IOSOCKET && TRACE_VERY_VERBOSE ) )( "%s - Checking socket changes\n", curr_local_time() );
        while( i < peer_list.get_count() )
        {
          cpeer = peer_list[i];
          if( cpeer )
            cpeer->manageConnections( &setr, &setw );
          i++;
        }

        // Use manageConnectingPeer to check the changes in the connecting sockets and do the necessary actions
        i = 0;
        while( i < connecting_peer_list.get_count() )
        {
          cpeer = connecting_peer_list[i];
          if( cpeer )
          {
            if( cpeer->manageConnectingPeer( &setr, &setw ) )
            {
              // Peer status changed, so we can add it to the peer_list
              TRACE( TRACE_IOSOCKETERROR )( "%s - New peer\n", curr_local_time() );
              connecting_peer_list.del( i ); // no risk of concurrent access issue with addConnectionPeer, since using an index
              peer_list.add( cpeer );
              i--;
            }
          }
          i++;
        }
      }
      else
      {
        if( res < 0 )
        {
          // There was an error in the select function
          TRACE( TRACE_IOSOCKETERROR )( "%s - select returned with error %d\n", curr_local_time(), socket_errno );

          if( socket_errno == EBADF
#ifdef WSAENOTSOCK
            || socket_errno == WSAENOTSOCK
#endif
            )
          {
            // There is a bad file descriptor, so we need to check the control socket and create a new one if necessary
#ifndef WIN32   
            if( control_socket.checkSocket( &setr ) >= 0 )
            {
              // There was no error with the control socket, so we must check the connections
              checkConnections();
            }
#else
            if( control_socket.checkSocket() >= 0 )
            {
              // There was no error with the control socket, so we must check the connections
              checkConnections();
            }
#endif              
          }
          else
          {
            // TODO EINTR a signal was caught before select returned
            // EINVAL the timeout was invalid or the number of file descriptors was invalid
            TRACE( TRACE_IOSOCKETERROR )( "%s - select returned with error EINTR or EINVAL\n", curr_local_time() );

            // On some other UNIX systems, select() can fail with the error
            // EAGAIN if the system fails to allocate kernel-internal resources,
            // rather than ENOMEM as Linux does. POSIX specifies this error for
            // poll(2), but not for select(). Portable programs may wish to
            // check for EAGAIN and loop, just as with EINTR.
          }
        }
        else
        {
          // res == 0 means that there was a timeout in the select function
          TRACE( ( TRACE_IOSOCKET && TRACE_VERY_VERBOSE ) )( "%s - select function finished with timeout\n", curr_local_time() );
        }
      }
    }
  }

  cleanup();
}

// Check all the connections in the peer_list and connecting_peer_list
void ThreadedConnectionManager::checkConnections()
{
  int i = 0;
  ConnectionPeer * cpeer = NULL;

  TRACE( ( TRACE_CONNECTIONS && TRACE_VERBOSE ) )( "%s - Checking connections in ThreadedConnectionManager\n", curr_local_time() );

  while( i < connecting_peer_list.get_count() )
  {
    cpeer = connecting_peer_list[i];
    if( cpeer )
    {
      cpeer->checkConnectingPeers();
    }
    i++;
  }

  i = 0;
  while( i < peer_list.get_count() )
  {
    cpeer = peer_list[i];
    if( cpeer )
    {
      cpeer->checkPeers();
    }
    i++;
  }
}

void ThreadedConnectionManager::cleanup()
{
  const ConnectionPeer * ptmp = NULL;

  while( connecting_peer_list.get_count() )
  {
    ptmp = connecting_peer_list[0];

    if( ptmp != NULL )
    {
      delete ptmp;
    }

    connecting_peer_list.del( 0 );
    ptmp = NULL;
  }

  while( peer_list.get_count() )
  {
    ptmp = peer_list[0];

    if( ptmp != NULL )
    {
      delete ptmp;
    }

    peer_list.del( 0 );
    ptmp = NULL;
  }

  while( to_be_closed.get_count() )
  {
    to_be_closed.del( 0 );
  }

}

int ThreadedConnectionManager::addConnectionPeer( ConnectionPeer * peer )
{
  //TODO LATER 1.1 add connection notification here ???
  currentconnections++;
  connecting_peer_list.add( peer );
  TRACE( ( TRACE_CONNECTIONS && TRACE_VERBOSE ) )( "%s - New peer added, now %d\n", curr_local_time(), connecting_peer_list.get_count() + peer_list.get_count() );
  control_socket.post();
  return 0;
}

void ThreadedConnectionManager::closePInfo( peer_info * pinfo )
{
  to_be_closed.add( pinfo );
  control_socket.post();
}

int ThreadedConnectionManager::getFreeConnections()
{
  TRACE( ( TRACE_CONNECTIONS && TRACE_VERBOSE ) )( "%s - Connection manager can handle up to %d connections, now %d\n", curr_local_time(), max_connections, currentconnections );

  return MAX( max_connections - currentconnections, 0 );
}

int ThreadedConnectionManager::getActiveConnections()
{
  return currentconnections;
}
