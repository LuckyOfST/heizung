#-------------------------------------------------
#
# Project created by QtCreator 2014-11-19T21:32:32
#
#-------------------------------------------------

QT       += core gui
QT += network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = HeizungGUI
TEMPLATE = app


SOURCES += main.cpp\
        Heizung.cpp \
    TempModel.cpp

HEADERS  += Heizung.h \
    TempModel.h

FORMS    += Heizung.ui
