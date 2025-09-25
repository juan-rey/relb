/*
   ControlSocket.cpp: ControlSocket class source file.

   Copyright 2006, 2007, 2008, 2009, 2025 Juan Rey Saura

This file is part of RELB Easy Load Balancer.

This software is a copyrighted work licensed under the Open Software License version 3.0
Please consult the file "LICENSE.txt" for details.
*/

#ifndef WIN32
#include <unistd.h> // Required for pipe(), read(), write(), close() on non-Windows systems
#endif

#include "ControlSocket.h"
#include "utiles.h"

USING_PTYPES

// Define a flag for non-blocking operations.
// MSG_DONTWAIT is a standard flag on Linux/BSD for non-blocking I/O.
// If it's not defined, NONBLOCKINGFLAGS is set to 0, falling back to default blocking behavior.
#ifndef MSG_DONTWAIT
#define NONBLOCKINGFLAGS 0
#else
#define NONBLOCKINGFLAGS MSG_DONTWAIT
#endif

/**
 * @brief Implements a control socket mechanism to wake up a waiting thread.
 * This class provides a cross-platform way to interrupt a blocking `select()` call.
 * - On Windows, it uses a socket-based "self-pipe" trick.
 * - On Unix-like systems, it uses a standard pipe.
 */
  ControlSocket::ControlSocket( void )
{
#ifdef WIN32
  // On Windows, create a standard TCP socket. This socket isn't meant to connect
  // to anything. It's used to signal a waiting thread by closing it, which causes
  // select() to unblock with an error.
  sock = socket( AF_INET, SOCK_STREAM, 0 );
  TRACE( ( TRACE_CONNECTIONS && TRACE_VERBOSE ) )( "%s - Creating control socket %d\n", curr_local_time(), sock );
#else
  // On non-Windows systems (Linux, macOS, etc.), create a pipe.
  // fd[0] is the read end, fd[1] is the write end.
  fd[0] = fd[1] = -1;
  if( pipe( fd ) == -1 )
  {
    // Error handling for pipe creation should be implemented here.
    // For example, logging the error or throwing an exception.
  }
#endif
}

/**
 * @brief Destructor for the ControlSocket. Cleans up the created handles.
 */
ControlSocket::~ControlSocket( void )
{
#ifdef WIN32
  // Close the Windows socket.
  ::closesocket( sock );
#else
  ::closesocket( fd[0] );
  ::closesocket( fd[1] );
#endif
}

/**
 * @brief Posts a signal to the control socket to wake up a waiting thread.
 * This is called from a different thread than the one calling select().
 */
void ControlSocket::post()
{
#ifdef WIN32
  // The Windows implementation is a "self-pipe" trick. To wake up the waiting
  // thread, we replace the current socket with a new one and close the old one.
  // The thread blocked in select() on the old socket will wake up because the
  // socket handle becomes invalid.

  int oldsock = sock;
  int tmpsock = -1;

  // Create a new socket to replace the old one.
  tmpsock = socket( AF_INET, SOCK_STREAM, 0 );
  TRACE( ( TRACE_CONNECTIONS && TRACE_VERBOSE ) )( "%s - Creating control socket %d\n", curr_local_time(), tmpsock );

  // This is a defensive check in case the OS reuses the socket descriptor immediately.
  // It's highly unlikely but ensures the new socket is different from the old one.
  if( tmpsock == oldsock )
  {
    tmpsock = socket( AF_INET, SOCK_STREAM, 0 );
    ::closesocket( oldsock ); // Close the original socket first
    TRACE( ( TRACE_CONNECTIONS && TRACE_VERBOSE ) )( "%s - Created control socket again, %d, old socket fd was identical to old one\n", curr_local_time(), tmpsock );
  }

  // Atomically replace the socket handle.
  sock = tmpsock;
  TRACE( ( TRACE_CONNECTIONS && TRACE_VERBOSE ) )( "%s - Closed old control socket %d\n", curr_local_time(), oldsock );

  // Now, close the old socket. This action will interrupt any select() call
  // that is monitoring `oldsock`.
  TRACE( ( TRACE_CONNECTIONS && TRACE_VERBOSE ) )( "%s - Closed old control socket %d\n", curr_local_time(), oldsock );
  if( ::closesocket( oldsock ) != 0 )
  {
    TRACE( ( TRACE_CONNECTIONS && TRACE_VERBOSE ) )( "%s - Could NOT close old control socket %d\n", curr_local_time(), oldsock );
  }
#else
  // On non-Windows systems, simply write one byte to the write-end of the pipe.
  // The thread select()ing on the read-end will wake up as data becomes available.
  // `buffer` is assumed to be a member variable, e.g., `char buffer[1];`
  int ret = write( fd[1], buffer, 1 );
  TRACE( ( TRACE_CONNECTIONS && TRACE_VERY_VERBOSE ) )( "%s - To control socket %d wrote %d\n", curr_local_time(), fd[0], ret );
#endif
}

/**
 * @brief Adds the control socket's file descriptor to a given fd_set.
 * This is used to prepare for a `select()` call.
 * @param set The file descriptor set to which the socket will be added.
 */
#ifdef ENABLE_SELECT_NFDS_CALC // see comment in utiles.h
void ControlSocket::addToFDSET( fd_set * set, int * p_maxfd )
#else
void ControlSocket::addToFDSET( fd_set * set )
#endif // ENABLE_SELECT_NFDS_CALC
{
#ifdef WIN32		
  // Check if the control socket is valid before adding it to the fd_set
  if( sock > 0 )
  {
    TRACE( ( TRACE_CONNECTIONS && TRACE_VERY_VERBOSE ) )( "%s - Adding to FDSET control socket %d\n", curr_local_time(), sock );
    FD_SET( sock, set );
  }
#else
  // Check if fd[0] is valid before adding it to the fd_set
  if( fd[0] > 0 )
  {
    TRACE( ( TRACE_CONNECTIONS && TRACE_VERY_VERBOSE ) )( "%s - Adding to FDSET control socket %d\n", curr_local_time(), fd[0] );
    FD_SET( fd[0], set );
#  ifdef ENABLE_SELECT_NFDS_CALC
    if( fd[0] > *p_maxfd )
    {
      *p_maxfd = fd[0];
      TRACE( ( TRACE_NFDS && TRACE_VERY_VERBOSE ) )( "%s - Updated nfds to %d\n", curr_local_time(), *p_maxfd );
    }
#  endif // ENABLE_SELECT_NFDS_CALC
  }
#endif
}

// Check the control socket for activity.
#ifdef WIN32	
int ControlSocket::checkSocket()
#else
int ControlSocket::checkSocket( fd_set * set )
#endif
{
  int ret = -1;
#ifdef WIN32		
  int tmpsock = sock;

  TRACE( ( TRACE_CONNECTIONS && TRACE_VERY_VERBOSE ) )( "%s - Checking control socket %d\n", curr_local_time(), tmpsock );

  if( tmpsock > 0 )
  {
    // Use select to check if the control socket is ready for reading or there is an error
    fd_set setr;
    timeval to;
    to.tv_sec = 0;
    to.tv_usec = 0;
    FD_ZERO( &setr );
    FD_SET( tmpsock, &setr );
    ret = ::select( tmpsock + 1, &setr, nil, nil, &to ); // select with timeout 0 to check immediately
    TRACE( ( TRACE_CONNECTIONS && TRACE_VERY_VERBOSE ) )( "%s - Select control socket %d returned %d\n", curr_local_time(), tmpsock, ret );
  }

  // If select returned an error, we need to close the socket and post a new one
  if( ret < 0 )
  {
    post();
  }
#else
  ret = 0;

  // Check if fd[0] is set in the fd_set, indicating that there is data to read
  if( FD_ISSET( fd[0], set ) )
  {
    // Read from fd[0] and return 1 if data is available
    ret = read( fd[0], buffer, 1 );
    TRACE( ( TRACE_CONNECTIONS && TRACE_VERY_VERBOSE ) )( "%s - Control socket %d read %d\n", curr_local_time(), fd[0], ret );
  }
#endif	

  return ret;
}
