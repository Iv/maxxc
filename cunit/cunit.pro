TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

QMAKE_CFLAGS +=-O2 -fopenmp -Wall -Wextra -Wno-unused -std=c99 -D_GNU_SOURCE
QMAKE_LFLAGS +=  -fopenmp

unix:!macx: LIBS += -lexpat -lm -lcunit

SOURCES += tests.c

