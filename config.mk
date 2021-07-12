X11INC = /usr/X11R6/include
X11LIB = /usr/X11R6/lib

PKG_CONFIG = pkg-config

# Xinerama (optional)
XINERAMALIBS = -lXinerama
XINERAMAFLAGS = -DXINERAMA

INCS = -I{X11INC} \
	   `$(PKG_CONFIG) --cflags freetype2`
LIBS = -L${X11LIB} ${XINERAMALIBS} -lX11 -lXft -lm \
	   `$(PKG_CONFIG) --libs freetype2`

CFLAGS = -std=c99 -pedantic -Wall -Wno-deprecated-declarations \
		 -Os ${INCS} ${XINERAMAFLAGS} -D_XOPEN_SOURCE=600
LDFLAGS = ${LIBS}

CC = cc
