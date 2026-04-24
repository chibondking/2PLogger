#pragma once
#include <QList>
#include <QObject>
#include <QString>
#include <cstdint>
#include <optional>
#include <string>
#include "log/QsoRecord.h"

namespace TwoPLogger {

// Owns the QSqlDatabase connection and all SQL strings.
// QSqlQuery and QSqlDatabase are never exposed through this interface.
class DatabaseManager : public QObject {
    Q_OBJECT
public:
    explicit DatabaseManager(QObject* parent = nullptr);
    ~DatabaseManager() override;

    // Connection lifecycle
    bool open(const QString& path);
    bool createSchema();
    int  schemaVersion();
    bool setSchemaVersion(int version);
    bool isOpen() const;
    void close();

    // Domain write operations
    std::optional<int64_t> insertLog(const LogRecord& rec);
    std::optional<int64_t> insertContest(const std::string& name,
                                         const std::string& cabrilloName);
    std::optional<int64_t> insertQso(const QsoRecord& qso);
    bool upsertCallsignHistory(const std::string& callsign,
                               int64_t contestId,
                               const std::optional<std::string>& name,
                               const std::optional<std::string>& spc,
                               const std::optional<std::string>& nr);

    // Domain read operations
    bool isDupe(const std::string& callsign,
                const std::string& band,
                int64_t logId);
    QList<QsoRecord>      fetchQsos(int64_t logId, int limit = -1);
    std::optional<LogRecord> fetchLog(int64_t logId);

private:
    bool    execSql(const QString& sql);
    QString m_connectionName;
    bool    m_open = false;
};

} // namespace TwoPLogger
