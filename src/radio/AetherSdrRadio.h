#pragma once
#include "radio/RigctldRadio.h"

namespace TwoPLogger {

// SpotPainter sends N1MM-format UDP spot packets to AetherSDR.
// Implementation is deferred to the UDP session.
class SpotPainter;

class AetherSdrRadio : public RigctldRadio {
    Q_OBJECT
public:
    explicit AetherSdrRadio(const RadioProfile& profile,
                             QObject* parent = nullptr);
    ~AetherSdrRadio() override;

private:
    SpotPainter* m_spotPainter = nullptr;  // owned; null until UDP session wires it up
};

} // namespace TwoPLogger
