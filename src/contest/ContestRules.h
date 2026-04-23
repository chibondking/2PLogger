#pragma once
#include <QObject>
#include <QString>
#include <QStringList>
#include <QMap>

namespace TwoPLogger {

struct QsoRecord;
class LogBook;

class ContestRules : public QObject {
    Q_OBJECT
public:
    explicit ContestRules(QObject* parent = nullptr);
    ~ContestRules() override;

    virtual QString     contestName()  const = 0;
    virtual QString     cabrilloName() const = 0;
    virtual QStringList exchangeFieldLabels() const = 0;
    virtual bool        isDupe(const QsoRecord& candidate, const LogBook& log) const = 0;
    virtual int         qsoPoints(const QsoRecord& qso) const = 0;
    virtual QString     validateExchange(const QsoRecord& qso) const = 0;
    virtual bool        updateMults(const QsoRecord& qso, LogBook& log) const = 0;
    virtual QString     formatCabrilloQso(const QsoRecord& qso) const = 0;
};

} // namespace TwoPLogger
