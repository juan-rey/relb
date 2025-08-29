/*
   Socket.h: socket class header file.

   Copyright 2006, 2007, 2008, 2009 Juan Rey Saura

This file is part of RELB Easy Load Balancer.

This software is a copyrighted work licensed under the Open Software License version 3.0
Please consult the file "LICENSE.txt" for details.
*/

#ifndef Socket_h
#define Socket_h

#include <pinet.h>
#include "ControlSocket.h"
#include "utiles.h"

USING_PTYPES

class Socket
{
protected:
  virtual void cleanup();

public:
  Socket( int sockfd, ipaddress * srcip, unsigned short srcport );
  Socket( const ipaddress * dstip, unsigned short dstport );
  Socket( const char * dsthost, int dstport );
  virtual ~Socket();

  int sendNB( const  char * buffer, int max );
  int receiveNB( char * buffer, int max );
  void close();
#ifdef DEBUG
  void close2();
#endif
  void getSocket();
  void addToFDSET( fd_set * set, int * p_nfds );
  int isInFDSET( fd_set * set );
  int checkSocket();
private:
  int sock;
  ipaddress ip;
  unsigned short port;
};
#endif
