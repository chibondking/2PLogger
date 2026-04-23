#pragma once
#include <QMainWindow>

namespace TwoPLogger {

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;
};

} // namespace TwoPLogger
