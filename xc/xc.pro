TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt
QMAKE_CFLAGS +=-O2 -fopenmp -Wall -Wextra -Wno-unused -std=c99 -D_GNU_SOURCE
QMAKE_LFLAGS +=  -fopenmp
TARGET = maxxc

SOURCES += ../maxxc.c ../track.c ../result.c ../declaration.c ../string_buffer.c 
HEADERS += ../maxxc.h

unix:!macx: LIBS += -lexpat -lm 

DISTFILES += \
    ../README \
    ../TODO \
    ../COPYING \
    ../maxxc-gpx2kml \
    ../maxxc-gpx2txt

