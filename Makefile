#PREFIX?=/usr/local
#BINDIR=${PREFIX}/bin
#MANDIR= ${PREFIX}/man/cat

PROG=	ffq
SRCS=	ffq.c
NOMAN=	Yes

CFLAGS+= -g -Wall -I${.CURDIR} -I../
CFLAGS+= -Wstrict-prototypes -Wmissing-prototypes
CFLAGS+= -Wmissing-declarations
CFLAGS+= -Wshadow -Wpointer-arith -Wcast-qual
CFLAGS+= -Wsign-compare
LDADD+= -lpthread

.include <bsd.prog.mk>
