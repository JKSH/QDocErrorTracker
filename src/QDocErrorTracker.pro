#-------------------------------------------------
#
# Project created by QtCreator 2014-01-12T18:54:49
#
#-------------------------------------------------

QT       += core gui sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = QDocErrorTracker
TEMPLATE = app
CONFIG += c++11

SOURCES += main.cpp \
    gui.cpp \
    database.cpp \
    fileselectiondialog.cpp

HEADERS  += \
    database.h \
    database_p.h \
    gui.h \
    gui_p.h \
    fileselectiondialog.h

FORMS    += \
    gui.ui \
    fileselectiondialog.ui
