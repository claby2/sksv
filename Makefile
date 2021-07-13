include config.mk

SRC = sksv.c
OBJ = ${SRC:.c=.o}

all: sksv

.c.o:
	${CC} -c ${CFLAGS} $<

sksv: ${OBJ}
	${CC} -o $@ ${OBJ} ${LDFLAGS}

clean:
	rm -rf sksv ${OBJ}

install: sksv
	mkdir -p ${PREFIX}/bin
	cp -f sksv ${PREFIX}/bin
	chmod 755 ${PREFIX}/bin/sksv

uninstall:
	rm -f ${PREFIX}/bin/sksv

.PHONY: all sksv clean
