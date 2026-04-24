#pragma once
#include "radio/RadioProfile.h"
#include <QObject>

namespace TwoPLogger {

class RadioInterface;

// Creates and connects a RadioInterface for the given profile.
// Returns AetherSdrRadio if profile.isAetherSdr, RigctldRadio otherwise.
// The caller (or parent) owns the returned object.
// connectToRigctld() is called automatically before returning.
RadioInterface* createRadio(const RadioProfile& profile,
                             QObject* parent = nullptr);

} // namespace TwoPLogger
