/*
   MessaggeQueue.h: MessaggeQueue class header file.

   Copyright 2006, 2007, 2008, 2009 Juan Rey Saura

This file is part of RELB Easy Load Balancer.

This software is a copyrighted work licensed under the Open Software License version 3.0
Please consult the file "LICENSE.txt" for details.
*/

#ifndef MessaggeQueue_h
#define MessaggeQueue_h

#include <pasync.h>
#include <pinet.h>

USING_PTYPES


class MessaggeQueue:
  public jobqueue
{
public:
  MessaggeQueue( void );
  ~MessaggeQueue( void );
  bool post( message * msg );
  bool post( int id, pintptr param = 0 );
  void setTrigger( timedsem * p );
private:
  timedsem * pts;

};


#endif
