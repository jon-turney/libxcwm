lib_LTLIBRARIES = libxcwm.la
AM_CFLAGS = $(XCB_CFLAGS) $(BASE_CFLAGS)

INCLUDES = -I${top_srcdir}/include

libxcwm_la_LDFLAGS = -version-number 0:0:0

libxcwm_la_LIBADD = $(XCB_LIBS)

libxcwm_la_SOURCES = \
	context.c \
	window.c \
	context_list.c \
	event_loop.c \
	init.c \
	util.c \
	image.c \
    input.c \
	atoms.c
