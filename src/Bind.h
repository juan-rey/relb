/*
   Bind.h: bind class header file.

   Copyright 2006, 2007, 2008, 2009 Juan Rey Saura

This file is part of Resolutive Easy Load Balancer.

This software is a copyrighted work licensed under the Open Software License version 3.0
Please consult the file "LICENSE.txt" for details.
*/

#ifndef Bind_h
#define Bind_h

#include <pasync.h>

USING_PTYPES

#include "ServerList.h"
#include "ThreadedConnectionManager.h"
#include "AdminHTTPServer.h"

class  Bind: public thread
{
protected:
  virtual void execute();
  virtual void cleanup();

public:
  Bind( /*unsigned short listeningPort, ipaddress listeningIP = ipnone,*/ int peers_per_thread = MAX_CONNECTIONS_PER_THREAD );
  virtual ~Bind();
  int startListening();
  int stopListening();
  bool addServer( const char * nombre, const ipaddress server_ip, unsigned short server_port, int weight = 0, int max_connections = 0 );
  void setAdmin( bool enable, const ipaddress server_ip, unsigned short server_port );
  bool addTask( TASK_TYPE type, int run_interval_ms );
  bool addTask( TASK_TYPE type, datetime firstrun, int run_interval_ms );
  bool addFilter( const ipaddress source_ip, const ipaddress source_mask, const ipaddress dest_ip, const ipaddress dest_mask, bool allow );
  void setServerRetry( int seconds );
  void setPeersPerThread( int ppt );
  void addAddress( unsigned short listeningPort, ipaddress listeningIP = ipnone );

private:
  AdminHTTPServer admin;
  tpodlist<bind_address *> addresses;
  //ipaddress ip;
  //unsigned short port;
  bool adminEnabled;
  ipaddress adminIP;
  unsigned short adminPort;
  bool finish;
  ServerList slist;
};
#endif
