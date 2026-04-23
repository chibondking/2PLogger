#include "app/MainWindow.h"

namespace TwoPLogger {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("2PLogger"));
    resize(1280, 800);
}

MainWindow::~MainWindow() = default;

} // namespace TwoPLogger
