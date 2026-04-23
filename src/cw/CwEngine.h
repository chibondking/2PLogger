#pragma once
#include <QObject>
#include <QString>

namespace TwoPLogger {

class RadioInterface;

class CwEngine : public QObject {
    Q_OBJECT
public:
    explicit CwEngine(RadioInterface* radio, QObject* parent = nullptr);
    ~CwEngine() override;

    void sendMessage(const QString& text);
    void abort();
    void setSpeed(int wpm);

    int speed() const;

signals:
    void speedChanged(int wpm);
    void sendingStateChanged(bool sending);

private:
    RadioInterface* m_radio;
    int             m_wpm = 28;
};

} // namespace TwoPLogger
