#include "log/LogBook.h"
#include "log/DatabaseManager.h"

namespace TwoPLogger {

LogBook::LogBook(QObject* parent)
    : QObject(parent)
    , m_db(std::make_unique<DatabaseManager>())
{}

LogBook::~LogBook() = default;

bool LogBook::open(const QString& path)
{
    return m_db->open(path) && m_db->createSchema();
}

void LogBook::close()
{
    m_db->close();
    m_currentLog.reset();
}

bool LogBook::isOpen() const
{
    return m_db->isOpen();
}

std::optional<int64_t> LogBook::createLog(const LogRecord& rec)
{
    auto id = m_db->insertLog(rec);
    if (id.has_value()) {
        LogRecord stored = rec;
        stored.id = *id;
        m_currentLog = stored;
    }
    return id;
}

std::optional<int64_t> LogBook::logQso(QsoRecord qso)
{
    using namespace std::chrono;
    if (qso.loggedAt == system_clock::time_point{})
        qso.loggedAt = system_clock::now();

    auto id = m_db->insertQso(qso);
    if (!id.has_value())
        return std::nullopt;

    qso.id = *id;

    // Update callsign history when logging in a contest session with exchange data.
    if (m_currentLog.has_value() && m_currentLog->contestId.has_value()
            && qso.contestData.has_value()) {
        const ContestData& cd = *qso.contestData;
        m_db->upsertCallsignHistory(
            qso.callsign,
            *m_currentLog->contestId,
            cd.exch1,
            cd.exch2,
            std::nullopt);
    }

    emit qsoLogged(qso);
    return id;
}

bool LogBook::isDupe(const QString& callsign, const QString& band, int logId)
{
    return m_db->isDupe(callsign.toStdString(), band.toStdString(),
                        static_cast<int64_t>(logId));
}

QList<QsoRecord> LogBook::recentQsos(int logId, int limit)
{
    return m_db->fetchQsos(static_cast<int64_t>(logId), limit);
}

QList<QsoRecord> LogBook::allQsos(int logId)
{
    return m_db->fetchQsos(static_cast<int64_t>(logId), -1);
}

} // namespace TwoPLogger
