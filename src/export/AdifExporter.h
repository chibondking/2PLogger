#pragma once
#include <QString>
#include <QList>

namespace TwoPLogger {

struct QsoRecord;

class AdifExporter {
public:
    AdifExporter();
    ~AdifExporter();

    bool    exportToFile(const QList<QsoRecord>& qsos, const QString& path) const;
    QString exportToString(const QList<QsoRecord>& qsos) const;
};

} // namespace TwoPLogger
