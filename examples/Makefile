ACALC_BIN=		acalc/acalc
ACALC_SRC=		acalc/acalc.c

INCS=			-I /usr/local/include
LIBS=			-L /usr/local/lib -laui
CFLAGS=			-O0

all: acalc

acalc:
	${CC} ${ACALC_SRC} -o ${ACALC_BIN} ${CFLAGS} ${LIBS} ${INCS}

clean:
	rm -rf ${ACALC_BIN} ${ACALC_BIN}.core

.PHONY: acalc
