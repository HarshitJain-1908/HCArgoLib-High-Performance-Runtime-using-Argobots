SUBDIRS =

# Includes folders
INCLUDES_DIR = -I$(top_srcdir)/inc -I$(top_srcdir)/src/inc -I$(ARGOBOTS_INSTALL_DIR)include

# cflags: important to define that otherwise we inherit default values too
CFLAGS = -Wall -g -O3 -std=c11
CXXFLAGS = -Wall -g -O3 -std=c++11
LDFLAGS = -labt -lpthread

# build the hclib library
lib_LTLIBRARIES = libhclib.la

# These are populated in sub-folder's Makefile.inc
noinst_LTLIBRARIES =
libhclib_la_LIBADD =

AM_CXXFLAGS = -L$(ARGOBOTS_INSTALL_DIR)lib -Wl,-rpath,$(ARGOBOTS_INSTALL_DIR)lib

libhclib_la_SOURCES = hcargolib-runtime.c hclib_cpp.cpp

# build hclib
libhclib_la_CXXFLAGS= $(CXXFLAGS) $(INCLUDES_DIR) $(AM_CXXFLAGS)
libhclib_la_CFLAGS= $(CFLAGS) $(INCLUDES_DIR) $(AM_CXXFLAGS)

MAINTAINERCLEANFILES = Makefile.in
