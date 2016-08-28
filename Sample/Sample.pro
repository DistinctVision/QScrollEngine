QT += widgets multimedia

TARGET = Sample
TEMPLATE = app

include (../QScrollEngine.pri)

SOURCES += main.cpp \
    scene.cpp

HEADERS += \ 
    scene.h

INCLUDEPATH += ../

RESOURCES += \
    $$PWD/resources.qrc



