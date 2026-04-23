#include <QApplication>
#include "app/MainWindow.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("2PLogger"));
    app.setApplicationVersion(QStringLiteral("0.1.0"));
    app.setOrganizationName(QStringLiteral("WT2P"));

    TwoPLogger::MainWindow window;
    window.show();

    return app.exec();
}
