QT       += core gui widgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET   = vyp_gui
TEMPLATE = app

CONFIG += c++17

SOURCES += \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    mainwindow.h

# macOS app bundle metadata
QMAKE_TARGET_BUNDLE_PREFIX = com.vyp
macx {
    QMAKE_APPLICATION_BUNDLE_NAME = "VYP Archiver"
}
