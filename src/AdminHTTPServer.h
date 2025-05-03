/*
   AdminHTTPServer.h: Web Admin class header file.

   Copyright 2006, 2007, 2008, 2009 Juan Rey Saura

This file is part of RELB Easy Load Balancer..

This software is a copyrighted work licensed under the Open Software License version 3.0
Please consult the file "LICENSE.txt" for details.
*/

#ifndef AdminHTTPServer_h
#define AdminHTTPServer_h

#include <pasync.h>

USING_PTYPES

#include "utiles.h"
#include "PeerInfo.h"
#include "MessaggeQueue.h"

class AdminHTTPServer: public thread
{
public:
  AdminHTTPServer();
  virtual ~AdminHTTPServer();
  void startHTTPAdmin( unsigned short port, const ipaddress ip );
  void stopHTTPAdmin();
  bool addServer( const char * nombre, const ipaddress * ip, unsigned short puerto, int weight = 0, int max_connections = 0 );
  bool addServer( const char * nombre, const char * hostname, unsigned short puerto, int weight = 0, int max_connections = 0 );
  void setPararllelList( const MessaggeQueue * jq );
  const MessaggeQueue * getJQ();

protected:
  virtual void execute();
  virtual void cleanup();
  void cleanServersList();

  //pages served
  void list( ipstream & client, ipaddress src_filter, ipaddress dst_filter, unsigned short src_port, unsigned short dst_port, int status_filter, int sort_by, bool ascending );
  void ban( ipstream & client, char * src_ip, char * src_port, bool undo = false );
  void ban_all( ipstream & client, char * src_ip, char * src_port, bool undo = false );
  void kick( ipstream & client, char * src_ip, char * src_port );
  void servers( ipstream & client );
  void redirect( ipstream & client );

  bool finish;
  tpodlist<peer_info *> peer_list;
  tpodlist<serverinfo *> server_list;
  MessaggeQueue jq;
  MessaggeQueue * parallellist_jq;
  unsigned short bind_port;
  ipaddress bind_ip;
};
#endif
