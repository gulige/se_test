QT += core
QT -= gui

CONFIG += c++11

TARGET = se_test
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

DEFINES += _UNICODE

QMAKE_CXXFLAGS_EXCEPTIONS_ON = /EHa
QMAKE_CXXFLAGS_STL_ON = /EHa

SOURCES += main.cpp \
    iexp/expstackwalker.cpp \
    iexp/iexp.cpp \
    iexp/ownexception.cpp \
    iexp/stackwalker.cpp

HEADERS += \
    iexp/expstackwalker.h \
    iexp/iexp.h \
    iexp/ownexception.h \
    iexp/stackwalker.h \
    iexp/stackwalkerinternal.h
