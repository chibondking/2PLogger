#include "dxcc/DxccDatabase.h"

namespace TwoPLogger {

DxccDatabase::DxccDatabase() = default;

DxccDatabase::~DxccDatabase() = default;

bool DxccDatabase::load(const QString& /*ctyDatPath*/) { return false; }

DxccEntity DxccDatabase::lookup(const QString& /*callsign*/) const
{
    return unknownEntity();
}

DxccEntity DxccDatabase::unknownEntity()
{
    DxccEntity e;
    e.name = QStringLiteral("Unknown");
    return e;
}

} // namespace TwoPLogger
