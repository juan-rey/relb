/*
   ControlSocket.h: ControlSocket class header file.

   Copyright 2006, 2007, 2008, 2009 Juan Rey Saura

This file is part of RELB Easy Load Balancer.

This software is a copyrighted work licensed under the Open Software License version 3.0
Please consult the file "LICENSE.txt" for details.
*/

#ifndef ControlSocket_h
#define ControlSocket_h

#include <pasync.h>
#include <pinet.h>

USING_PTYPES

/// <summary>
///  ControlSocket class, used to control the ThreadedConnectionManager.
///  It is a socket that can be used to wake up the ThreadedConnectionManager
///  when it is waiting for connections.
/// </summary>
/// if WIN32 uses a socket, otherwise uses a pipe.
class ControlSocket
{
public:
  ControlSocket( void );
  ~ControlSocket( void );
  void post();
  void addToFDSET( fd_set * set, int * p_maxfd );
#ifdef WIN32	
  int checkSocket();
#else
  int checkSocket( fd_set * set );
#endif	
private:
#ifdef WIN32	
  int sock;
#else
  int fd[2];
  char buffer[1];
#endif
};

#endif
