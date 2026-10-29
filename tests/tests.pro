QT += widgets testlib
CONFIG += testcase c++11
TARGET = tst_mainwindow
INCLUDEPATH += ..
SOURCES += tst_mainwindow.cpp ../mainwindow.cpp
HEADERS += ../mainwindow.h
FORMS += ../mainwindow.ui
