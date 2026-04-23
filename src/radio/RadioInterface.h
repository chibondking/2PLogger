#pragma once
#include <QObject>
#include <QString>

namespace TwoPLogger {

enum class RadioMode { CW, SSB, FT8, RTTY, Unknown };

class RadioInterface : public QObject {
    Q_OBJECT
public:
    explicit RadioInterface(QObject* parent = nullptr);
    ~RadioInterface() override;

    virtual void qsy(quint64 hz) = 0;
    virtual void setMode(RadioMode mode) = 0;
    virtual void setPtt(bool tx) = 0;
    virtual void sendCw(const QString& text) = 0;
    virtual void abortCw() = 0;

signals:
    void frequencyChanged(quint64 hz);
    void modeChanged(RadioMode mode);
    void txStateChanged(bool transmitting);
    void connectionStateChanged(bool connected);
};

} // namespace TwoPLogger
