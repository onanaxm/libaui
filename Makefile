CC=			cc
LIB=		libaui
PREFIX=		/usr/local

SRCS=		button.c \
			canvas.c \
			driver.c \
			frame.c \
			font.c \
			layout.c \
			ui.c \
			xcb.c \
			widget.c \
			window.c
CFLAGS+=	-fPIC -O0 -std=c99 -Werror
OBJS=		${SRCS:.c=.o}
INCS=		-I /usr/X11R6/include -I /usr/X11R6/include/freetype2
HDR=		aui.h
LDADD=		-L /usr/X11R6/lib -lxcb -lxcb-render -lfontconfig -lfreetype


all: ${LIB}


${LIB}: ${OBJS}
	${INSTALL} -m 755 ${HDR} ${PREFIX}/include
	${CC} ${OBJS} -shared -o ${PREFIX}/lib/${LIB}.so ${LDADD}
	ar rc ${PREFIX}/lib/${LIB}.a ${OBJS}

.c.o:
	${CC} -c -fPIC $< -o $@ ${INCS} ${CFLAGS}

clean:
	rm -rf ${PREFIX}/lib/${LIB}.so ${PREFIX}/include/${HDR} ${PREFIX}/lib/${LIB}.a ${OBJS}
