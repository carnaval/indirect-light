TEMPLATE = app

QT += qml quick widgets opengl

SOURCES += main.cpp \
    screen.cpp \
    save.cpp

RESOURCES += qml.qrc

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

# Default rules for deployment.
include(deployment.pri)

HEADERS += \
    screen.h \
    map.h \
    geom.h \
    save.h

OTHER_FILES += \
    a.vs \
    std.vs \
    std.fs \
    lit.fs \
    bounce.fs \
    light.fs \
    lightseg.fs \
    synth.fs \
    flat.fs \
    synth_seg.fs \
    synth_bal.fs \
    lightbal.fs
