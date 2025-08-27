/*
   ServerList.h: server list class header file.

   Copyright 2006, 2007, 2008, 2009, 2025 Juan Rey Saura

This file is part of RELB Easy Load Balancer.

This software is a copyrighted work licensed under the Open Software License version 3.0
Please consult the file "LICENSE.txt" for details.
*/

#ifndef ServerList_h
#define ServerList_h

#include <pasync.h>
#include <pinet.h>

USING_PTYPES

#include "utiles.h"
#include "ThreadedConnectionManagerList.h"
#include "MessageQueue.h"

class ServerList: public thread
{
protected:
  virtual void execute();
  virtual void cleanup();
  void processMessage( message * msg );
  // Internal state
  bool update;
  bool finish;
  bool reconnect_lost_sessions;
  ThreadedConnectionManagerList cmlist;
  tpodlist<filterinfo *> filter_list;
  tpodlist<serverinfo *> servers_list;
  rwlock servers_lock;
  tpodlist<peer_info *> peer_list;
  rwlock peer_lock;
  tpodlist<task_info *> tasks_list;
  datetime next_task_run_time;
  MessageQueue * parallelList;
  MessageQueue jq;
  timedsem status;
  //TODO
  //int updateServerInfo();
  int cleanConnections();
  int purgeConnections();
  void cleanList();
  void cleanPeerList();
  void cleanTaskList();
  int last_server_assigned;
  int server_retry_milliseconds;
  int finished_weight;
  int disconnected_weight;
  datetime last_connections_cleanup;
public:
  void setPeersPerThread( int ppt );
  void setServerRetry( int seconds );
  ServerList();
  bool addServer( const char * host_name, const ipaddress * ip, unsigned short port, int weight = 0, int max_connections = 0 );
  //bool addServer( const char * name, const char * host_name, unsigned short port, int weight = 0, int max_connections = 0 );
  bool addTask( TASK_TYPE task_type, int run_interval_seconds );
  bool addTask( TASK_TYPE task_type, datetime firstrun, int run_interval_seconds );
  bool addFilter( const ipaddress source_ip, const ipaddress source_mask, const ipaddress dest_ip, const ipaddress dest_mask, bool allow );
  const MessageQueue * getQueue();
  const peer_info * getServer( const ipaddress * client_ip, unsigned short client_port, ThreadedConnectionManager * pcm );
  void setServer( int client_socket, sockaddr_in * sac );
  virtual ~ServerList();
  virtual int startUpdating();
  virtual int stopUpdating();
  void setParallelList( const MessageQueue * jq );
  bool isServerAllowed( const ipaddress * server, const ipaddress * client );
};
#endif
