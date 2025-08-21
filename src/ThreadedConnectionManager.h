/*
   ThreadedConnectionManager.h: connection manager header file.

   Copyright 2006, 2007, 2008, 2009 Juan Rey Saura

This file is part of RELB Easy Load Balancer.

This software is a copyrighted work licensed under the Open Software License version 3.0
Please consult the file "LICENSE.txt" for details.
*/

#ifndef ThreadedConnectionManager_h
#define ThreadedConnectionManager_h

#include <pasync.h>

USING_PTYPES

#include "ConnectionPeerList.h"
#include "ControlSocket.h"

#ifdef FD_SETSIZE
#define MAX_PEERS_PER_FDSET (( FD_SETSIZE - 2 )/ 2 ) // 2 for control socket and 2 for each peer
#else
#define MAX_PEERS_PER_FDSET ( 60 / 2 )
#endif

#define DEFAULT_TCM_MAX_CONNECTIONS MAX_PEERS_PER_FDSET

class ThreadedConnectionManager: public thread
{
protected:
  virtual void execute();
  virtual void cleanup();
public:
  ThreadedConnectionManager( int connections = DEFAULT_TCM_MAX_CONNECTIONS );
  virtual ~ThreadedConnectionManager();
  virtual int addConectionPeer( ConnectionPeer * peer );
  void closePInfo( peer_info * pinfo );
  int getFreeConnections();
  int getActiveConnections();
private:
  void checkConnections();

  tobjlist<ConnectionPeer> peer_list;
  tobjlist<ConnectionPeer> connecting_peer_list;
  tobjlist<peer_info> to_be_closed;
  int max_connections;
  int currentconnections;
  bool finish;
  ControlSocket control_socket;
};
#endif
