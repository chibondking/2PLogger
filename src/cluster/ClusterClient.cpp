#include "cluster/ClusterClient.h"

namespace TwoPLogger {

ClusterClient::ClusterClient(QObject* parent)
    : QObject(parent)
{}

ClusterClient::~ClusterClient() = default;

void ClusterClient::connectToHost(const QString& /*host*/, quint16 /*port*/) {}

void ClusterClient::disconnectFromHost() {}

} // namespace TwoPLogger
