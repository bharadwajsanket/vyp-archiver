#include <QApplication>
#include <QPalette>
#include "mainwindow.h"

// Detect whether the host OS / desktop environment is using a dark colour
// scheme by inspecting the default window background luminance.
// Qt sets the application palette from the system theme before main() runs,
// so this is available immediately.
static bool systemPrefersDark()
{
    const QColor bg = QApplication::palette().color(QPalette::Window);
    // Perceived luminance (ITU-R BT.601 coefficients).
    const double lum = 0.299 * bg.redF()
                     + 0.587 * bg.greenF()
                     + 0.114 * bg.blueF();
    return lum < 0.5;   // dark background → dark theme
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    app.setApplicationName("VYP Archiver");
    app.setApplicationVersion("1.0");
    app.setOrganizationName("VYP");

    MainWindow window(systemPrefersDark());
    window.show();

    return app.exec();
}