#include "cw/CwEngine.h"

namespace TwoPLogger {

CwEngine::CwEngine(RadioInterface* radio, QObject* parent)
    : QObject(parent)
    , m_radio(radio)
{}

CwEngine::~CwEngine() = default;

void CwEngine::sendMessage(const QString& /*text*/) {}

void CwEngine::abort() {}

void CwEngine::setSpeed(int wpm)
{
    if (m_wpm == wpm)
        return;
    m_wpm = wpm;
    emit speedChanged(m_wpm);
}

int CwEngine::speed() const { return m_wpm; }

} // namespace TwoPLogger
