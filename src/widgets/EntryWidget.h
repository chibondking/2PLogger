#pragma once
#include <QWidget>

namespace TwoPLogger {

class EntryWidget : public QWidget {
    Q_OBJECT
public:
    explicit EntryWidget(QWidget* parent = nullptr);
    ~EntryWidget() override;

    QString callsign() const;
    void    clearFields();

signals:
    void qsoReadyToLog();
    void callsignChanged(const QString& callsign);
};

} // namespace TwoPLogger
