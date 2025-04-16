/*
   MessaggeQueue.cpp: MessaggeQueue class source file.

   Copyright 2006, 2007, 2008, 2009 Juan Rey Saura

This file is part of Resolutive Easy Load Balancer.

This software is a copyrighted work licensed under the Open Software License version 3.0
Please consult the file "LICENSE.txt" for details.
*/

#include "MessaggeQueue.h"
#include "utiles.h"

USING_PTYPES

MessaggeQueue::MessaggeQueue( void )
{
  pts = NULL;
}

MessaggeQueue::~MessaggeQueue( void )
{
}

bool MessaggeQueue::post( message * msg )
{
  TRACE( TRACE_VERBOSE )( "%s - sending a message with a post()\n", curr_local_time() );

  bool val = jobqueue::post( msg );

  if( pts )
    pts->post();

  return val;
}


bool MessaggeQueue::post( int id, pintptr param )
{
  TRACE( TRACE_VERBOSE )( "%s - sending a message with a post()\n", curr_local_time() );
  bool val = jobqueue::post( id, param );

  if( pts )
    pts->post();

  return val;
}

void MessaggeQueue::setTrigger( timedsem * p )
{
  pts = p;
}
