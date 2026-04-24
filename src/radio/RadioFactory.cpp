#include "radio/RadioFactory.h"
#include "radio/AetherSdrRadio.h"
#include "radio/RigctldRadio.h"

namespace TwoPLogger {

RadioInterface* createRadio(const RadioProfile& profile, QObject* parent)
{
    RigctldRadio* radio = profile.isAetherSdr
        ? static_cast<RigctldRadio*>(new AetherSdrRadio(profile, parent))
        : new RigctldRadio(profile, parent);

    radio->connectToRigctld();
    return radio;
}

} // namespace TwoPLogger
