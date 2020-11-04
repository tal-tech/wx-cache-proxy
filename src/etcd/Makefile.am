MAINTAINERCLEANFILES = Makefile.in

AM_CPPFLAGS = -I $(top_srcdir)/src

AM_CFLAGS = -Wall -Wshadow
AM_CFLAGS += -Wno-unused-parameter -Wno-unused-value

noinst_LIBRARIES = libtimer.a

#libtimer_a_LIBADD=-lcurl -lpthread  -lrt -lzlog
#noinst_HEADERS = nc_hashkit.h



libtimer_a_SOURCES =		\
	timer.c		\
	etcd_conf.c		\
	cetcd.c		\
	cetcd_array.c		\
	sds.c		\
	cJSON.c		\
	encode.c	\
	log.c
