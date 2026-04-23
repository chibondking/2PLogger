#pragma once
#include <QObject>
#include <QString>
#include <QDateTime>

namespace TwoPLogger {

struct QsoRecord {
    qint64    id = 0;
    QString   callsign;
    quint64   freqHz = 0;
    QString   band;
    QString   mode;
    QString   rstSent  = QStringLiteral("599");
    QString   rstRcvd  = QStringLiteral("599");
    QString   dxccEntity;
    QString   continent;
    int       cqZone   = 0;
    int       ituZone  = 0;
    QString   exch1;
    QString   exch2;
    QDateTime timestampUtc;
    bool      isDupe   = false;
};

class LogBook : public QObject {
    Q_OBJECT
public:
    explicit LogBook(QObject* parent = nullptr);
    ~LogBook() override;

    bool open(const QString& path);
    void close();
    bool isOpen() const;
};

} // namespace TwoPLogger
