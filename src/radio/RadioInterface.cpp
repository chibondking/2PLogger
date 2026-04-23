#include "radio/RadioInterface.h"

namespace TwoPLogger {

RadioInterface::RadioInterface(QObject* parent)
    : QObject(parent)
{}

RadioInterface::~RadioInterface() = default;

} // namespace TwoPLogger
