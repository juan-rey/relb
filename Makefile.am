## Process this file with automake to produce Makefile.in
## Created by Anjuta

SUBDIRS = src 

relbdocdir = ${prefix}/doc/relb
relbdoc_DATA = \
	README\
	COPYING\
	AUTHORS\
	ChangeLog\
	INSTALL\
	NEWS


EXTRA_DIST = $(relbdoc_DATA)

AM_CXXFLAGS = \
	-D_DEBUG \
	-D_GNU_SOURCE \
	-I${prefix}/../../ptypes/include \
	-pthread

INCLUDES = \
	${prefix}/../../ptypes/lib/libptypes.a

# Remove doc directory on uninstall
uninstall-local:
	-rm -r $(relbdocdir)
