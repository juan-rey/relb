/*
   ThreadedConnectionManager.h: connection manager list header file.

   Copyright 2006, 2007, 2008, 2009 Juan Rey Saura

This file is part of RELB Easy Load Balancer..

This software is a copyrighted work licensed under the Open Software License version 3.0
Please consult the file "LICENSE.txt" for details.
*/

#ifndef ThreadedConnectionManagerList_h
#define ThreadedConnectionManagerList_h

#include <pasync.h>
#include <ptypes.h>

USING_PTYPES

#include "ThreadedConnectionManager.h"

#define MAX_CONNECTIONS_PER_THREAD DEFAULT_TCM_MAX_CONNECTIONS

class ThreadedConnectionManagerList
{
public:
  ThreadedConnectionManagerList();
  virtual ~ThreadedConnectionManagerList();
  ThreadedConnectionManager * getFreeThreadedConnectionManager();
  void setPeersPerThread( int ppt );
private:
  ThreadedConnectionManager * addNew();
  void cleanList();
  void purifyList();
  int peers_per_thread;
  tpodlist<ThreadedConnectionManager *> m_list;
};
#endif
