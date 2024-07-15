## Process this file with automake to produce Makefile.in

## Created by Anjuta

AM_CPPFLAGS = \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" 

AM_CFLAGS =\
	 -Wall\
	 -g

bin_PROGRAMS = relb

relb_SOURCES = \
	main.cpp\
	MessaggeQueue.cpp \
	MessaggeQueue.h \
	PeerInfo.cpp \
	PeerInfo.h \
	relb.h \
	ServerList.cpp \
	ServerList.h \
	Socket.cpp \
	Socket.h \
	ThreadedConnectionManager.cpp \
	ThreadedConnectionManager.h \
	ThreadedConnectionManagerList.cpp \
	ThreadedConnectionManagerList.h \
	utiles.cpp \
	utiles.h \
	win32service.cpp \
	win32service.h \
	AdminHTTPServer.cpp \
	AdminHTTPServer.h \
	AppConfig.cpp \
	AppConfig.h \
	Bind.cpp \
	Bind.h \
	ConnectionPeer.cpp \
	ConnectionPeer.h \
	ConnectionPeerList.cpp \
	ConnectionPeerList.h \
	ControlObject.cpp \
	ControlObject.h \
	htmldefs.h \
	license.h

relb_LDADD = \
	${srcdir}/../../ptypes/lib/libptypes.a

relb_CXXFLAGS = \
	-D_GNU_SOURCE \
	-I${srcdir}/../../ptypes/include \
	-pthread

