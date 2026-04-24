#pragma once
#include "radio/RadioMode.h"
#include "radio/RadioProfile.h"
#include <QObject>
#include <QString>

namespace TwoPLogger {

class RadioInterface : public QObject {
    Q_OBJECT
public:
    explicit RadioInterface(QObject* parent = nullptr) : QObject(parent) {}
    ~RadioInterface() override = default;

    virtual void qsy(quint64 hz)            = 0;
    virtual void setMode(RadioMode mode)    = 0;
    virtual void setPtt(bool tx)            = 0;
    virtual void sendCw(const QString& text) = 0;
    virtual void abortCw()                  = 0;
    virtual bool isConnected() const        = 0;
    virtual RadioProfile profile() const    = 0;

signals:
    void frequencyChanged(quint64 hz);
    void modeChanged(RadioMode mode);
    void txStateChanged(bool transmitting);
    void connectionStateChanged(bool connected);
    void errorOccurred(QString message);
};

} // namespace TwoPLogger
