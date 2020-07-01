TEMPLATE = lib
CONFIG += plugin qmltypes c++11
QT += qml quick

QML_IMPORT_NAME = QuickVR
QML_IMPORT_MAJOR_VERSION = 1

DESTDIR = imports/$$QML_IMPORT_NAME
TARGET  = quickvrplugin
QMLTYPES_FILENAME = $$DESTDIR/plugins.qmltypes

_OVR_SDK_ROOT = $$(OVR_SDK_ROOT)
isEmpty(_OVR_SDK_ROOT) {
    error("OVR_SDK_ROOT" environment variable not set.)
} else {
    DEFINES += HAVE_LIBOVR
    INCLUDEPATH += \
        $$_OVR_SDK_ROOT/LibOVR/Include \
        ovr

    CONFIG( debug, debug|release ):_OVR_CONFIG=Debug
    else:_OVR_CONFIG=Release

    contains(QT_ARCH, i386): _OVR_ARCH=Win32
    else: _OVR_ARCH=x64

    LIBS += -L$$_OVR_SDK_ROOT/LibOVR/Lib/Windows/$$_OVR_ARCH/$$_OVR_CONFIG/VS2017 -lLibOVR

    SOURCES += \
        ovr/VRRenderer.cpp

    HEADERS += \
        ovr/VRRenderer.h
}

SOURCES += \
        QuickVR_plugin.cpp \
        VRHeadset.cpp \
        VRWindow.cpp

HEADERS += \
        QuickVR_plugin.h \
        VRHeadset.h \
        VRWindow.h

PLUGINFILES= \
    imports/$$QML_IMPORT_NAME/qmldir

target.path = $$[QT_INSTALL_QML]/$$QML_IMPORT_NAME

pluginfiles_copy.files = $$PLUGINFILES
pluginfiles_copy.path = $$DESTDIR

INSTALLS += target
COPIES += pluginfiles_copy

OTHER_FILES += \
    $$PLUGINFILES

CONFIG += install_ok  # Do not cargo-cult this!
