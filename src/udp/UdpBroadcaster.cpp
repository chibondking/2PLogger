#include "udp/UdpBroadcaster.h"

namespace TwoPLogger {

UdpBroadcaster::UdpBroadcaster(QObject* parent)
    : QObject(parent)
{}

UdpBroadcaster::~UdpBroadcaster() = default;

void UdpBroadcaster::setTarget(const QString& /*host*/, quint16 /*port*/) {}

void UdpBroadcaster::broadcast(const QJsonObject& /*packet*/) {}

} // namespace TwoPLogger
