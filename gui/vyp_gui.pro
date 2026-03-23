QT       += core gui widgets

CONFIG   += c++17

TARGET   = "VYP Archiver"
TEMPLATE = app

SOURCES += \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    mainwindow.h

# macOS app bundle metadata
QMAKE_TARGET_BUNDLE_PREFIX = com.vyp
macx {
    QMAKE_APPLICATION_BUNDLE_NAME = "VYP Archiver"

    # ── Copy vypack engine into the app bundle AFTER linking ──
    # Runs as a post-link step so the .app/Contents/MacOS/ dir already exists.
    QMAKE_POST_LINK += test -f $$shell_quote($$PWD/../vypack) && \
        cp -f $$shell_quote($$PWD/../vypack) \
              $$shell_quote($$OUT_PWD/VYP Archiver.app/Contents/MacOS/vypack) && \
        chmod +x $$shell_quote($$OUT_PWD/VYP Archiver.app/Contents/MacOS/vypack) && \
        echo \"Bundled vypack engine into app\" || \
        echo \"WARNING: ../vypack not found — build engine with make first\"
}
