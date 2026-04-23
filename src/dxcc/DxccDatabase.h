#pragma once
#include <QString>

namespace TwoPLogger {

struct DxccEntity {
    QString name;
    QString prefix;
    QString continent;
    int     cqZone    = 0;
    int     ituZone   = 0;
    double  lat       = 0.0;
    double  lon       = 0.0;
    float   utcOffset = 0.0f;
    bool    isDeleted = false;
};

class DxccDatabase {
public:
    DxccDatabase();
    ~DxccDatabase();

    bool       load(const QString& ctyDatPath);
    DxccEntity lookup(const QString& callsign) const;

    static DxccEntity unknownEntity();
};

} // namespace TwoPLogger
