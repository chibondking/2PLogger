#pragma once
#include <QObject>
#include <QJsonObject>
#include <QString>

namespace TwoPLogger {

class UdpBroadcaster : public QObject {
    Q_OBJECT
public:
    explicit UdpBroadcaster(QObject* parent = nullptr);
    ~UdpBroadcaster() override;

    void setTarget(const QString& host, quint16 port);
    void broadcast(const QJsonObject& packet);
};

} // namespace TwoPLogger
