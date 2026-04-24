#include "log/DatabaseManager.h"

#include <QDateTime>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>
#include <QtGlobal>
#include <atomic>

namespace TwoPLogger {

// ── connection name generator ────────────────────────────────────────────────

static QString makeConnectionName()
{
    static std::atomic<int> counter{0};
    return QStringLiteral("2plogger_%1").arg(++counter);
}

// ── ISO-8601 UTC helpers (used only inside this TU) ─────────────────────────

static QString toIso8601(const std::chrono::system_clock::time_point& tp)
{
    auto secs = std::chrono::duration_cast<std::chrono::seconds>(
                    tp.time_since_epoch())
                    .count();
    return QDateTime::fromSecsSinceEpoch(static_cast<qint64>(secs), Qt::UTC)
        .toString(Qt::ISODate);
}

static std::chrono::system_clock::time_point fromIso8601(const QString& s)
{
    if (s.isEmpty())
        return {};
    QDateTime dt = QDateTime::fromString(s, Qt::ISODate);
    if (!dt.isValid())
        return {};
    dt.setTimeSpec(Qt::UTC);
    return std::chrono::system_clock::from_time_t(
        static_cast<time_t>(dt.toSecsSinceEpoch()));
}

// ── QVariant helpers for std::optional ──────────────────────────────────────

static QVariant optVar(const std::optional<int>& v)
{
    return v.has_value() ? QVariant(*v) : QVariant();
}

static QVariant optVar(const std::optional<int64_t>& v)
{
    return v.has_value() ? QVariant(static_cast<qint64>(*v)) : QVariant();
}

static QVariant optStrVar(const std::optional<std::string>& v)
{
    return v.has_value() ? QVariant(QString::fromStdString(*v)) : QVariant();
}

// ── DatabaseManager ──────────────────────────────────────────────────────────

DatabaseManager::DatabaseManager(QObject* parent)
    : QObject(parent)
    , m_connectionName(makeConnectionName())
{}

DatabaseManager::~DatabaseManager()
{
    close();
    QSqlDatabase::removeDatabase(m_connectionName);
}

bool DatabaseManager::execSql(const QString& sql)
{
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    if (!q.exec(sql)) {
        qWarning() << "SQL error:" << q.lastError().text() << "\n  SQL:" << sql;
        return false;
    }
    return true;
}

// ── Connection lifecycle ─────────────────────────────────────────────────────

bool DatabaseManager::open(const QString& path)
{
    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    db.setDatabaseName(path);
    if (!db.open()) {
        qWarning() << "Cannot open database:" << db.lastError().text();
        return false;
    }
    execSql(QStringLiteral("PRAGMA foreign_keys = ON"));
    execSql(QStringLiteral("PRAGMA journal_mode = WAL"));
    m_open = true;
    return true;
}

bool DatabaseManager::createSchema()
{
    // Tables must be created in FK dependency order:
    //   contest → log → qso → qso_contest_data
    //                       → contest_mult
    //   contest → callsign_history

    static const QStringList ddl = {
        // contest
        QStringLiteral(R"(
            CREATE TABLE IF NOT EXISTS contest (
                id            INTEGER PRIMARY KEY AUTOINCREMENT,
                name          TEXT NOT NULL UNIQUE,
                cabrillo_name TEXT,
                def_file      TEXT
            )
        )"),

        // log
        QStringLiteral(R"(
            CREATE TABLE IF NOT EXISTS log (
                id             INTEGER PRIMARY KEY AUTOINCREMENT,
                created_at     TEXT NOT NULL,
                name           TEXT NOT NULL,
                station_call   TEXT NOT NULL,
                contest_id     INTEGER REFERENCES contest(id),
                my_grid        TEXT,
                my_dxcc        TEXT,
                my_cq_zone     INTEGER,
                operator_class TEXT,
                power_w        INTEGER,
                radio_profile  TEXT
            )
        )"),

        // qso
        QStringLiteral(R"(
            CREATE TABLE IF NOT EXISTS qso (
                id            INTEGER PRIMARY KEY AUTOINCREMENT,
                log_id        INTEGER NOT NULL REFERENCES log(id),
                timestamp_utc TEXT NOT NULL,
                callsign      TEXT NOT NULL COLLATE NOCASE,
                freq_hz       INTEGER NOT NULL,
                band          TEXT NOT NULL,
                mode          TEXT NOT NULL,
                rst_sent      TEXT NOT NULL DEFAULT '599',
                rst_rcvd      TEXT NOT NULL DEFAULT '599',
                tx_power_w    INTEGER,
                dxcc_entity   TEXT,
                continent     TEXT,
                cq_zone       INTEGER,
                itu_zone      INTEGER,
                operator_call TEXT,
                notes         TEXT,
                is_dupe       INTEGER NOT NULL DEFAULT 0,
                is_deleted    INTEGER NOT NULL DEFAULT 0,
                logged_at     TEXT NOT NULL
            )
        )"),

        // qso_contest_data
        QStringLiteral(R"(
            CREATE TABLE IF NOT EXISTS qso_contest_data (
                qso_id      INTEGER PRIMARY KEY REFERENCES qso(id) ON DELETE CASCADE,
                exch1       TEXT,
                exch2       TEXT,
                exch3       TEXT,
                exch4       TEXT,
                serial_sent INTEGER,
                serial_rcvd INTEGER,
                points      INTEGER NOT NULL DEFAULT 0,
                is_mult     INTEGER NOT NULL DEFAULT 0,
                mult_type   TEXT,
                mult_value  TEXT
            )
        )"),

        // contest_mult
        QStringLiteral(R"(
            CREATE TABLE IF NOT EXISTS contest_mult (
                id           INTEGER PRIMARY KEY AUTOINCREMENT,
                log_id       INTEGER NOT NULL REFERENCES log(id),
                mult_type    TEXT NOT NULL,
                mult_value   TEXT NOT NULL COLLATE NOCASE,
                first_qso_id INTEGER REFERENCES qso(id),
                UNIQUE(log_id, mult_type, mult_value)
            )
        )"),

        // callsign_history
        QStringLiteral(R"(
            CREATE TABLE IF NOT EXISTS callsign_history (
                callsign   TEXT NOT NULL COLLATE NOCASE,
                contest_id INTEGER NOT NULL REFERENCES contest(id),
                last_name  TEXT,
                last_spc   TEXT,
                last_nr    TEXT,
                last_seen  TEXT NOT NULL,
                PRIMARY KEY (callsign, contest_id)
            )
        )"),

        // schema_version (single-row, keyed by rowid)
        QStringLiteral(R"(
            CREATE TABLE IF NOT EXISTS schema_version (
                version INTEGER NOT NULL DEFAULT 0
            )
        )"),

        // Indexes (verbatim from CLAUDE.md)
        QStringLiteral("CREATE INDEX IF NOT EXISTS idx_qso_log_band  ON qso(log_id, band)"),
        QStringLiteral("CREATE INDEX IF NOT EXISTS idx_qso_log_call  ON qso(log_id, callsign)"),
        QStringLiteral("CREATE INDEX IF NOT EXISTS idx_qso_timestamp ON qso(timestamp_utc)"),
        QStringLiteral("CREATE INDEX IF NOT EXISTS idx_mult_log       ON contest_mult(log_id, mult_type)"),
    };

    for (const QString& stmt : ddl) {
        if (!execSql(stmt))
            return false;
    }

    // Seed schema_version if table is empty.
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery q(db);
    q.exec(QStringLiteral("SELECT COUNT(*) FROM schema_version"));
    if (q.next() && q.value(0).toInt() == 0)
        execSql(QStringLiteral("INSERT INTO schema_version VALUES (1)"));

    return true;
}

int DatabaseManager::schemaVersion()
{
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    if (!q.exec(QStringLiteral("SELECT version FROM schema_version LIMIT 1")))
        return 0;
    return q.next() ? q.value(0).toInt() : 0;
}

bool DatabaseManager::setSchemaVersion(int version)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery q(db);
    q.prepare(QStringLiteral("UPDATE schema_version SET version = :v"));
    q.bindValue(QStringLiteral(":v"), version);
    return q.exec();
}

bool DatabaseManager::isOpen() const
{
    return m_open && QSqlDatabase::database(m_connectionName).isOpen();
}

void DatabaseManager::close()
{
    if (QSqlDatabase::contains(m_connectionName)) {
        QSqlDatabase db = QSqlDatabase::database(m_connectionName);
        if (db.isOpen())
            db.close();
    }
    m_open = false;
}

// ── Domain writes ────────────────────────────────────────────────────────────

std::optional<int64_t> DatabaseManager::insertLog(const LogRecord& rec)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    if (!db.isOpen())
        return std::nullopt;

    QSqlQuery q(db);
    q.prepare(QStringLiteral(R"(
        INSERT INTO log (
            created_at, name, station_call, contest_id,
            my_grid, my_dxcc, my_cq_zone, operator_class, power_w, radio_profile
        ) VALUES (
            :created_at, :name, :station_call, :contest_id,
            :my_grid, :my_dxcc, :my_cq_zone, :operator_class, :power_w, :radio_profile
        )
    )"));

    auto createdAt = rec.createdAt == std::chrono::system_clock::time_point{}
                         ? std::chrono::system_clock::now()
                         : rec.createdAt;

    q.bindValue(QStringLiteral(":created_at"),    toIso8601(createdAt));
    q.bindValue(QStringLiteral(":name"),          QString::fromStdString(rec.name));
    q.bindValue(QStringLiteral(":station_call"),  QString::fromStdString(rec.stationCall));
    q.bindValue(QStringLiteral(":contest_id"),    optVar(rec.contestId));
    q.bindValue(QStringLiteral(":my_grid"),       optStrVar(rec.myGrid));
    q.bindValue(QStringLiteral(":my_dxcc"),       optStrVar(rec.myDxcc));
    q.bindValue(QStringLiteral(":my_cq_zone"),    optVar(rec.myCqZone));
    q.bindValue(QStringLiteral(":operator_class"),optStrVar(rec.operatorClass));
    q.bindValue(QStringLiteral(":power_w"),       optVar(rec.powerW));
    q.bindValue(QStringLiteral(":radio_profile"), optStrVar(rec.radioProfile));

    if (!q.exec()) {
        qWarning() << "insertLog failed:" << q.lastError().text();
        return std::nullopt;
    }
    return static_cast<int64_t>(q.lastInsertId().toLongLong());
}

std::optional<int64_t> DatabaseManager::insertContest(const std::string& name,
                                                       const std::string& cabrilloName)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    if (!db.isOpen())
        return std::nullopt;

    QSqlQuery q(db);
    q.prepare(QStringLiteral(R"(
        INSERT OR IGNORE INTO contest (name, cabrillo_name) VALUES (:name, :cabrillo)
    )"));
    q.bindValue(QStringLiteral(":name"),     QString::fromStdString(name));
    q.bindValue(QStringLiteral(":cabrillo"), QString::fromStdString(cabrilloName));
    if (!q.exec())
        return std::nullopt;

    // May have been ignored (duplicate). Fetch the id either way.
    QSqlQuery sel(db);
    sel.prepare(QStringLiteral("SELECT id FROM contest WHERE name = :name"));
    sel.bindValue(QStringLiteral(":name"), QString::fromStdString(name));
    if (!sel.exec() || !sel.next())
        return std::nullopt;
    return static_cast<int64_t>(sel.value(0).toLongLong());
}

std::optional<int64_t> DatabaseManager::insertQso(const QsoRecord& qso)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    if (!db.isOpen())
        return std::nullopt;

    if (!db.transaction())
        return std::nullopt;

    QSqlQuery q(db);
    q.prepare(QStringLiteral(R"(
        INSERT INTO qso (
            log_id, timestamp_utc, callsign, freq_hz, band, mode,
            rst_sent, rst_rcvd, tx_power_w, dxcc_entity, continent,
            cq_zone, itu_zone, operator_call, notes,
            is_dupe, is_deleted, logged_at
        ) VALUES (
            :log_id, :timestamp_utc, :callsign, :freq_hz, :band, :mode,
            :rst_sent, :rst_rcvd, :tx_power_w, :dxcc_entity, :continent,
            :cq_zone, :itu_zone, :operator_call, :notes,
            :is_dupe, :is_deleted, :logged_at
        )
    )"));

    q.bindValue(QStringLiteral(":log_id"),        static_cast<qint64>(qso.logId));
    q.bindValue(QStringLiteral(":timestamp_utc"), toIso8601(qso.timestampUtc));
    q.bindValue(QStringLiteral(":callsign"),      QString::fromStdString(qso.callsign));
    q.bindValue(QStringLiteral(":freq_hz"),       static_cast<qint64>(qso.freqHz));
    q.bindValue(QStringLiteral(":band"),          QString::fromStdString(qso.band));
    q.bindValue(QStringLiteral(":mode"),          QString::fromStdString(qso.mode));
    q.bindValue(QStringLiteral(":rst_sent"),      QString::fromStdString(qso.rstSent));
    q.bindValue(QStringLiteral(":rst_rcvd"),      QString::fromStdString(qso.rstRcvd));
    q.bindValue(QStringLiteral(":tx_power_w"),    optVar(qso.txPowerW));
    q.bindValue(QStringLiteral(":dxcc_entity"),   optStrVar(qso.dxccEntity));
    q.bindValue(QStringLiteral(":continent"),     optStrVar(qso.continent));
    q.bindValue(QStringLiteral(":cq_zone"),       optVar(qso.cqZone));
    q.bindValue(QStringLiteral(":itu_zone"),      optVar(qso.ituZone));
    q.bindValue(QStringLiteral(":operator_call"), optStrVar(qso.operatorCall));
    q.bindValue(QStringLiteral(":notes"),         optStrVar(qso.notes));
    q.bindValue(QStringLiteral(":is_dupe"),       qso.isDupe    ? 1 : 0);
    q.bindValue(QStringLiteral(":is_deleted"),    qso.isDeleted ? 1 : 0);
    q.bindValue(QStringLiteral(":logged_at"),     toIso8601(qso.loggedAt));

    if (!q.exec()) {
        qWarning() << "insertQso failed:" << q.lastError().text();
        db.rollback();
        return std::nullopt;
    }

    int64_t qsoId = static_cast<int64_t>(q.lastInsertId().toLongLong());

    if (qso.contestData.has_value()) {
        const ContestData& cd = *qso.contestData;
        QSqlQuery cq(db);
        cq.prepare(QStringLiteral(R"(
            INSERT INTO qso_contest_data (
                qso_id, exch1, exch2, exch3, exch4,
                serial_sent, serial_rcvd, points, is_mult, mult_type, mult_value
            ) VALUES (
                :qso_id, :exch1, :exch2, :exch3, :exch4,
                :serial_sent, :serial_rcvd, :points, :is_mult, :mult_type, :mult_value
            )
        )"));
        cq.bindValue(QStringLiteral(":qso_id"),      static_cast<qint64>(qsoId));
        cq.bindValue(QStringLiteral(":exch1"),       optStrVar(cd.exch1));
        cq.bindValue(QStringLiteral(":exch2"),       optStrVar(cd.exch2));
        cq.bindValue(QStringLiteral(":exch3"),       optStrVar(cd.exch3));
        cq.bindValue(QStringLiteral(":exch4"),       optStrVar(cd.exch4));
        cq.bindValue(QStringLiteral(":serial_sent"), optVar(cd.serialSent));
        cq.bindValue(QStringLiteral(":serial_rcvd"), optVar(cd.serialRcvd));
        cq.bindValue(QStringLiteral(":points"),      cd.points);
        cq.bindValue(QStringLiteral(":is_mult"),     cd.isMult ? 1 : 0);
        cq.bindValue(QStringLiteral(":mult_type"),   optStrVar(cd.multType));
        cq.bindValue(QStringLiteral(":mult_value"),  optStrVar(cd.multValue));

        if (!cq.exec()) {
            qWarning() << "insertQso/contest_data failed:" << cq.lastError().text();
            db.rollback();
            return std::nullopt;
        }
    }

    if (!db.commit()) {
        db.rollback();
        return std::nullopt;
    }
    return qsoId;
}

bool DatabaseManager::upsertCallsignHistory(const std::string& callsign,
                                             int64_t contestId,
                                             const std::optional<std::string>& name,
                                             const std::optional<std::string>& spc,
                                             const std::optional<std::string>& nr)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    if (!db.isOpen())
        return false;

    QSqlQuery q(db);
    q.prepare(QStringLiteral(R"(
        INSERT OR REPLACE INTO callsign_history
            (callsign, contest_id, last_name, last_spc, last_nr, last_seen)
        VALUES
            (:callsign, :contest_id, :last_name, :last_spc, :last_nr, :last_seen)
    )"));
    q.bindValue(QStringLiteral(":callsign"),   QString::fromStdString(callsign));
    q.bindValue(QStringLiteral(":contest_id"), static_cast<qint64>(contestId));
    q.bindValue(QStringLiteral(":last_name"),  optStrVar(name));
    q.bindValue(QStringLiteral(":last_spc"),   optStrVar(spc));
    q.bindValue(QStringLiteral(":last_nr"),    optStrVar(nr));
    q.bindValue(QStringLiteral(":last_seen"),
                QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    return q.exec();
}

// ── Domain reads ─────────────────────────────────────────────────────────────

bool DatabaseManager::isDupe(const std::string& callsign,
                              const std::string& band,
                              int64_t logId)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    if (!db.isOpen())
        return false;

    QSqlQuery q(db);
    q.prepare(QStringLiteral(R"(
        SELECT COUNT(*) FROM qso
        WHERE log_id = :log_id
          AND callsign = :callsign COLLATE NOCASE
          AND band = :band
          AND is_deleted = 0
    )"));
    q.bindValue(QStringLiteral(":log_id"),   static_cast<qint64>(logId));
    q.bindValue(QStringLiteral(":callsign"), QString::fromStdString(callsign));
    q.bindValue(QStringLiteral(":band"),     QString::fromStdString(band));
    if (!q.exec() || !q.next())
        return false;
    return q.value(0).toInt() > 0;
}

QList<QsoRecord> DatabaseManager::fetchQsos(int64_t logId, int limit)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QList<QsoRecord> result;
    if (!db.isOpen())
        return result;

    // Column indices match the SELECT list below exactly.
    // q.*: 0-18, cd.*: 19-28
    QString sql = QStringLiteral(R"(
        SELECT q.id, q.log_id, q.timestamp_utc, q.callsign, q.freq_hz, q.band, q.mode,
               q.rst_sent, q.rst_rcvd, q.tx_power_w, q.dxcc_entity, q.continent,
               q.cq_zone, q.itu_zone, q.operator_call, q.notes,
               q.is_dupe, q.is_deleted, q.logged_at,
               cd.exch1, cd.exch2, cd.exch3, cd.exch4,
               cd.serial_sent, cd.serial_rcvd, cd.points, cd.is_mult,
               cd.mult_type, cd.mult_value
        FROM qso q
        LEFT JOIN qso_contest_data cd ON cd.qso_id = q.id
        WHERE q.log_id = :log_id AND q.is_deleted = 0
        ORDER BY q.timestamp_utc DESC
    )");
    if (limit > 0)
        sql += QStringLiteral(" LIMIT %1").arg(limit);

    QSqlQuery q(db);
    q.prepare(sql);
    q.bindValue(QStringLiteral(":log_id"), static_cast<qint64>(logId));
    if (!q.exec())
        return result;

    while (q.next()) {
        QsoRecord rec;
        rec.id            = q.value(0).toLongLong();
        rec.logId         = q.value(1).toLongLong();
        rec.timestampUtc  = fromIso8601(q.value(2).toString());
        rec.callsign      = q.value(3).toString().toStdString();
        rec.freqHz        = static_cast<uint64_t>(q.value(4).toLongLong());
        rec.band          = q.value(5).toString().toStdString();
        rec.mode          = q.value(6).toString().toStdString();
        rec.rstSent       = q.value(7).toString().toStdString();
        rec.rstRcvd       = q.value(8).toString().toStdString();
        if (!q.isNull(9))  rec.txPowerW    = q.value(9).toInt();
        if (!q.isNull(10)) rec.dxccEntity  = q.value(10).toString().toStdString();
        if (!q.isNull(11)) rec.continent   = q.value(11).toString().toStdString();
        if (!q.isNull(12)) rec.cqZone      = q.value(12).toInt();
        if (!q.isNull(13)) rec.ituZone     = q.value(13).toInt();
        if (!q.isNull(14)) rec.operatorCall= q.value(14).toString().toStdString();
        if (!q.isNull(15)) rec.notes       = q.value(15).toString().toStdString();
        rec.isDupe        = q.value(16).toInt() != 0;
        rec.isDeleted     = q.value(17).toInt() != 0;
        rec.loggedAt      = fromIso8601(q.value(18).toString());

        // cd.points (col 25) is NULL on a LEFT JOIN miss, 0+ when a row exists.
        if (!q.isNull(25)) {
            ContestData cd;
            cd.qsoId = rec.id;
            if (!q.isNull(19)) cd.exch1      = q.value(19).toString().toStdString();
            if (!q.isNull(20)) cd.exch2      = q.value(20).toString().toStdString();
            if (!q.isNull(21)) cd.exch3      = q.value(21).toString().toStdString();
            if (!q.isNull(22)) cd.exch4      = q.value(22).toString().toStdString();
            if (!q.isNull(23)) cd.serialSent = q.value(23).toInt();
            if (!q.isNull(24)) cd.serialRcvd = q.value(24).toInt();
            cd.points  = q.value(25).toInt();
            cd.isMult  = q.value(26).toInt() != 0;
            if (!q.isNull(27)) cd.multType   = q.value(27).toString().toStdString();
            if (!q.isNull(28)) cd.multValue  = q.value(28).toString().toStdString();
            rec.contestData = cd;
        }

        result.append(rec);
    }
    return result;
}

std::optional<LogRecord> DatabaseManager::fetchLog(int64_t logId)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    if (!db.isOpen())
        return std::nullopt;

    QSqlQuery q(db);
    q.prepare(QStringLiteral(R"(
        SELECT id, created_at, name, station_call, contest_id,
               my_grid, my_dxcc, my_cq_zone, operator_class, power_w, radio_profile
        FROM log WHERE id = :id
    )"));
    q.bindValue(QStringLiteral(":id"), static_cast<qint64>(logId));
    if (!q.exec() || !q.next())
        return std::nullopt;

    LogRecord rec;
    rec.id          = q.value(0).toLongLong();
    rec.createdAt   = fromIso8601(q.value(1).toString());
    rec.name        = q.value(2).toString().toStdString();
    rec.stationCall = q.value(3).toString().toStdString();
    if (!q.isNull(4))  rec.contestId     = static_cast<int64_t>(q.value(4).toLongLong());
    if (!q.isNull(5))  rec.myGrid        = q.value(5).toString().toStdString();
    if (!q.isNull(6))  rec.myDxcc        = q.value(6).toString().toStdString();
    if (!q.isNull(7))  rec.myCqZone      = q.value(7).toInt();
    if (!q.isNull(8))  rec.operatorClass = q.value(8).toString().toStdString();
    if (!q.isNull(9))  rec.powerW        = q.value(9).toInt();
    if (!q.isNull(10)) rec.radioProfile  = q.value(10).toString().toStdString();
    return rec;
}

} // namespace TwoPLogger
