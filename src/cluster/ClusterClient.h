#pragma once
#include <QObject>
#include <QString>

namespace TwoPLogger {

class ClusterClient : public QObject {
    Q_OBJECT
public:
    explicit ClusterClient(QObject* parent = nullptr);
    ~ClusterClient() override;

    void connectToHost(const QString& host, quint16 port);
    void disconnectFromHost();

signals:
    void connectionStateChanged(bool connected);
    void rawLineReceived(const QString& line);
};

} // namespace TwoPLogger
