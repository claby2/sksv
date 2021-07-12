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

.PHONY: all sksv clean
