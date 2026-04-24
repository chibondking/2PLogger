#pragma once
#include <QList>
#include <QObject>
#include <QString>
#include <memory>
#include <optional>
#include "log/QsoRecord.h"

namespace TwoPLogger {

class DatabaseManager;

class LogBook : public QObject {
    Q_OBJECT
public:
    explicit LogBook(QObject* parent = nullptr);
    ~LogBook() override;

    bool open(const QString& path);
    void close();
    bool isOpen() const;

    std::optional<int64_t> createLog(const LogRecord& rec);
    std::optional<int64_t> logQso(QsoRecord qso);  // fills loggedAt; emits qsoLogged
    bool isDupe(const QString& callsign, const QString& band, int logId);
    QList<QsoRecord> recentQsos(int logId, int limit = 50);
    QList<QsoRecord> allQsos(int logId);

signals:
    void qsoLogged(const QsoRecord& qso);

private:
    std::unique_ptr<DatabaseManager> m_db;
    std::optional<LogRecord>         m_currentLog;
};

} // namespace TwoPLogger
