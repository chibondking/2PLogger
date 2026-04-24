#include "radio/AetherSdrRadio.h"

namespace TwoPLogger {

AetherSdrRadio::AetherSdrRadio(const RadioProfile& profile, QObject* parent)
    : RigctldRadio(profile, parent)
{}

AetherSdrRadio::~AetherSdrRadio() = default;

} // namespace TwoPLogger
