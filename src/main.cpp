#include <cstdint>
#include <cmath>
#include <array>
#include <vector>
#include <algorithm>
#include <QApplication>
#include <QIcon>
#include "ui/LobbyWidget.h"

int main(int argc, char *argv[]) {
#if defined(Q_OS_LINUX)
    // Force X11 / XWayland backend so QCursor::setPos works for 360-degree FPS mouse capture
    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")) {
        qputenv("QT_QPA_PLATFORM", "xcb");
    }
#endif

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif

    QApplication app(argc, argv);
    app.setApplicationName("VanguardFPS");
    app.setApplicationDisplayName("VanguardFPS - Underground Weapons Facility");

    LobbyWidget lobbyWidget;
    lobbyWidget.show();

    return app.exec();
}
