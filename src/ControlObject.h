/* 
   ControlObject.h: ControlObject class header file.

   Copyright 2006, 2007, 2008, 2009 Juan Rey Saura

This file is part of Resolutive Easy Load Balancer.

This software is a copyrighted work licensed under the Open Software License version 3.0
Please consult the file "LICENSE.txt" for details.
*/

#ifndef ControlObject_h
#define ControlObject_h

#include <pasync.h>
#include <pinet.h>

USING_PTYPES


class ControlObject
{
public:
	ControlObject(void);
	~ControlObject(void);
	virtual void post() = 0;
};

class ControlSocket: public ControlObject
{
public:
	ControlSocket(void);
	~ControlSocket(void);
	void post();
	void addToFDSET( fd_set * set );
#ifdef WIN32	
	int checkSocket( );
#else
	int checkSocket( fd_set * set  );
#endif	
private:
#ifdef WIN32	
	int sock;
#else
	int fd[2];
	char buffer[1];
#endif
};

#endif
