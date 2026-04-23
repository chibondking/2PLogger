#include "export/AdifExporter.h"
#include "log/LogBook.h"

namespace TwoPLogger {

AdifExporter::AdifExporter() = default;

AdifExporter::~AdifExporter() = default;

bool AdifExporter::exportToFile(const QList<QsoRecord>& /*qsos*/,
                                const QString& /*path*/) const
{
    return false;
}

QString AdifExporter::exportToString(const QList<QsoRecord>& /*qsos*/) const
{
    return {};
}

} // namespace TwoPLogger
