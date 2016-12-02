#-------------------------------------------------
#
# Project created by QtCreator 2015-03-27T13:21:18
#
#-------------------------------------------------


TARGET = opening_hours
TEMPLATE = lib
CONFIG += staticlib

ROOT_DIR = ../..

win32* {
QMAKE_CXXFLAGS += -O1 -ftrack-macro-expansion=0 -Wl,--large--address-aware -Wa,-mbig-obj
}

include($$ROOT_DIR/common.pri)

HEADERS += opening_hours.hpp \
           opening_hours_parsers.hpp \
           opening_hours_parsers_terminals.hpp \
           parse_opening_hours.hpp \
           rules_evaluation_private.hpp \
           rules_evaluation.hpp

SOURCES += rules_evaluation.cpp \
           opening_hours.cpp \
           parse_opening_hours.cpp
