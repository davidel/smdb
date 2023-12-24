
INCLUDES = -I../include -I. -I..

noinst_PROGRAMS = smdbtest

smdbtest_SOURCES = smdb-test.c smdb-xif-posix.c
smdbtest_CFLAGS = $(AM_CFLAGS) -DHAVE_SMDB_CONFIG_H
smdbtest_LDADD = ../src/.libs/libsmdb.a

