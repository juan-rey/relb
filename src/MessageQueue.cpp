/*
   MessageQueue.cpp: MessageQueue class source file.

   Copyright 2006, 2007, 2008, 2009 Juan Rey Saura

This file is part of RELB Easy Load Balancer.

This software is a copyrighted work licensed under the Open Software License version 3.0
Please consult the file "LICENSE.txt" for details.
*/

#include "MessageQueue.h"
#include "utiles.h"

USING_PTYPES

MessageQueue::MessageQueue( void )
{
  pts = NULL;
}

MessageQueue::~MessageQueue( void )
{
}

bool MessageQueue::post( message * msg )
{
  TRACE( TRACE_UNCATEGORIZED && TRACE_VERBOSE )( "%s - sending a message with a post()\n", curr_local_time() );

  bool val = jobqueue::post( msg );

  if( pts )
    pts->post();

  return val;
}


bool MessageQueue::post( int id, pintptr param )
{
  TRACE( TRACE_UNCATEGORIZED && TRACE_VERBOSE )( "%s - sending a message with a post()\n", curr_local_time() );
  bool val = jobqueue::post( id, param );

  if( pts )
    pts->post();

  return val;
}

void MessageQueue::setTrigger( timedsem * p )
{
  pts = p;
}
