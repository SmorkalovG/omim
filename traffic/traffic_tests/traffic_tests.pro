TARGET = traffic_tests
CONFIG += console warn_on
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../..

INCLUDEPATH *= $$ROOT_DIR/3party/jansson/src

DEPENDENCIES = traffic editor routing_common indexer platform_tests_support platform coding geometry base stats_client protobuf jansson

include($$ROOT_DIR/common.pri)

DEFINES *= OMIM_UNIT_TEST_WITH_QT_EVENT_LOOP

QT *= core

macx-* {
  QT *= widgets # needed for QApplication with event loop, to test async events
  LIBS *= "-framework IOKit" "-framework SystemConfiguration"
}

win*|linux* {
  QT *= network
}

win32* {
  LIBS += -lshlwapi
}

SOURCES += \
    $$ROOT_DIR/testing/testingmain.cpp \
    traffic_info_test.cpp \
