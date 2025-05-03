/*
   ThreadedConnectionManager.cpp: connection manager list source file.

   Copyright 2006, 2007, 2008,2009 Juan Rey Saura

This file is part of RELB Easy Load Balancer..

This software is a copyrighted work licensed under the Open Software License version 3.0
Please consult the file "LICENSE.txt" for details.
*/

#include "ThreadedConnectionManagerList.h"

USING_PTYPES

ThreadedConnectionManagerList::ThreadedConnectionManagerList()
{
  peers_per_thread = MAX_CONNECTIONS_PER_THREAD;
}

ThreadedConnectionManagerList::~ThreadedConnectionManagerList()
{
  cleanList();
}

void ThreadedConnectionManagerList::cleanList()
{
  const ThreadedConnectionManager * pcm = NULL;

  while( m_list.get_count() )
  {
    pcm = m_list[0];

    if( pcm != NULL )
    {
      delete pcm;
    }

    m_list.del( 0 );
    pcm = NULL;
  }
}

void ThreadedConnectionManagerList::purifyList()
{
  int i = 0;

  while( i < m_list.get_count() )
  {
    if( m_list[i]->getActiveConnections() == 0 )
    {
      delete m_list[i];
      m_list.del( i );
      i--;
    }
    i++;
  }
}

void ThreadedConnectionManagerList::setPeersPerThread( int ppt )
{
  if( ppt > 0 )
    peers_per_thread = ppt;
}

ThreadedConnectionManager * ThreadedConnectionManagerList::getFreeThreadedConnectionManager()
{
  ThreadedConnectionManager * pcm = NULL;

  if( m_list.get_count() == 0 )
  {
    pcm = addNew();
  }
  else
  {
    int i = 1;
    int freeconections = -1;//lista[0]->getFreeConnections();
    pcm = NULL;
    int current = 0;//lista[0];

    while( i < m_list.get_count() )
    {
      current = m_list[i]->getFreeConnections();
      if( current > freeconections && current > 0 )
      {
        freeconections = current;
        pcm = m_list[i];
      }
      i++;
    }

    if( freeconections < 1 || !pcm )
    {
      pcm = addNew();
    }
  }

  return pcm;
}


ThreadedConnectionManager * ThreadedConnectionManagerList::addNew()
{
  TRACE( TRACE_VERY_VERBOSE )( "%s - creating new connection manager to handle up to %d connections\n", curr_local_time(), peers_per_thread );
  m_list.add( new ThreadedConnectionManager( peers_per_thread ) );
  return m_list.top();
}
