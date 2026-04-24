#include "radio/RigctldRadio.h"
#include <QAbstractSocket>
#include <QTcpSocket>
#include <QtGlobal>

namespace TwoPLogger {

// ── construction ─────────────────────────────────────────────────────────────

RigctldRadio::RigctldRadio(const RadioProfile& profile, QObject* parent)
    : RadioInterface(parent)
    , m_profile(profile)
    , m_socket(new QTcpSocket(this))
{
    connect(m_socket, &QTcpSocket::connected,
            this, &RigctldRadio::onConnected);
    connect(m_socket, &QTcpSocket::disconnected,
            this, &RigctldRadio::onDisconnected);
    connect(m_socket, &QTcpSocket::errorOccurred,
            this, &RigctldRadio::onSocketError);
    connect(m_socket, &QTcpSocket::readyRead,
            this, &RigctldRadio::onReadyRead);

    m_pollTimer.setInterval(profile.pollIntervalMs);
    connect(&m_pollTimer, &QTimer::timeout,
            this, &RigctldRadio::onPollTick);

    m_reconnectTimer.setSingleShot(true);
    connect(&m_reconnectTimer, &QTimer::timeout,
            this, &RigctldRadio::onReconnectTick);
}

RigctldRadio::~RigctldRadio()
{
    m_intentionalDisconnect = true;
    m_reconnectTimer.stop();
    m_pollTimer.stop();
    if (m_socket->state() != QAbstractSocket::UnconnectedState)
        m_socket->disconnectFromHost();
}

// ── public API ────────────────────────────────────────────────────────────────

void RigctldRadio::connectToRigctld()
{
    m_intentionalDisconnect = false;
    m_reconnectDelay        = 1000;
    m_socket->connectToHost(
        QString::fromStdString(m_profile.rigctldHost),
        m_profile.rigctldPort);
}

void RigctldRadio::disconnectFromRigctld()
{
    m_intentionalDisconnect = true;
    m_reconnectTimer.stop();
    m_pollTimer.stop();
    m_socket->disconnectFromHost();
}

void RigctldRadio::qsy(quint64 hz)
{
    // Set VFO to A first, then tune the frequency.
    enqueueCommand(CmdType::SetVfo,  QByteArray("+\\set_vfo VFOA\n"));
    enqueueCommand(CmdType::SetFreq, QByteArray("F ") + QByteArray::number(hz) + "\n");
}

void RigctldRadio::setMode(RadioMode mode)
{
    // Map DIGI → PKTUSB for the rigctld wire format.
    const char* wire = (mode == RadioMode::DIGI) ? "PKTUSB"
                     : modeToString(mode).c_str();
    enqueueCommand(CmdType::SetMode,
                   QByteArray("M ") + wire + " 0\n");
}

void RigctldRadio::setPtt(bool tx)
{
    enqueueCommand(CmdType::SetPtt,
                   QByteArray("T ") + (tx ? "1" : "0") + "\n");
}

void RigctldRadio::sendCw(const QString& text)
{
    enqueueCommand(CmdType::SendMorse,
                   QByteArray("+\\send_morse ")
                   + text.toUtf8() + "\n");
}

void RigctldRadio::abortCw()
{
    enqueueCommand(CmdType::StopMorse,
                   QByteArray("+\\stop_morse\n"));
}

// ── private slots ─────────────────────────────────────────────────────────────

void RigctldRadio::onConnected()
{
    m_connected      = true;
    m_busy           = false;
    m_reconnectDelay = 1000;
    m_readBuf.clear();
    m_pendingLines.clear();
    m_queue.clear();
    m_pollTimer.start();
    emit connectionStateChanged(true);
}

void RigctldRadio::onDisconnected()
{
    handleDisconnect();
}

void RigctldRadio::onSocketError(QAbstractSocket::SocketError /*err*/)
{
    // errorOccurred fires before disconnected; skip if we already started reconnect.
    if (!m_reconnectTimer.isActive() && !m_intentionalDisconnect) {
        qWarning("RigctldRadio: socket error: %s",
                 qPrintable(m_socket->errorString()));
        emit errorOccurred(m_socket->errorString());
        handleDisconnect();
    }
}

void RigctldRadio::onReadyRead()
{
    m_readBuf += m_socket->readAll();

    // Process as many complete responses as are buffered.
    bool foundRprt = true;
    while (foundRprt) {
        foundRprt = false;

        int nlIdx;
        while ((nlIdx = m_readBuf.indexOf('\n')) != -1) {
            QString line = QString::fromUtf8(
                m_readBuf.left(nlIdx)).trimmed();
            m_readBuf.remove(0, nlIdx + 1);

            if (line.isEmpty()) continue;
            m_pendingLines << line;

            if (line.startsWith(QStringLiteral("RPRT"))) {
                if (m_busy) {
                    processResponse(m_pendingLines);
                    m_busy = false;
                }
                m_pendingLines.clear();
                drain();
                foundRprt = true;
                break;
            }
        }
    }
}

void RigctldRadio::onPollTick()
{
    // Only enqueue a fresh poll if the queue is empty and nothing is in flight.
    // This prevents poll commands from piling up if the rig is slow to respond.
    if (m_queue.isEmpty() && !m_busy) {
        m_queue.enqueue({CmdType::GetFreq, "f\n"});
        m_queue.enqueue({CmdType::GetMode, "m\n"});
        drain();
    }
}

void RigctldRadio::onReconnectTick()
{
    connectToRigctld();
}

// ── private helpers ───────────────────────────────────────────────────────────

void RigctldRadio::enqueueCommand(CmdType type, const QByteArray& data)
{
    if (!m_connected) return;
    m_queue.enqueue({type, data});
    drain();
}

void RigctldRadio::drain()
{
    if (m_busy || m_queue.isEmpty()) return;
    m_inFlight = m_queue.dequeue();
    m_busy     = true;
    m_socket->write(m_inFlight.data);
}

void RigctldRadio::processResponse(const QStringList& lines)
{
    // lines.last() is always the "RPRT <n>" line.
    const QString& rprtLine = lines.last();
    bool  ok   = false;
    int   rprt = rprtLine.mid(5).toInt(&ok);   // "RPRT " is 5 chars
    bool  success = ok && (rprt == 0);

    if (!success && m_inFlight.type != CmdType::GetFreq
                 && m_inFlight.type != CmdType::GetMode) {
        emit errorOccurred(
            QStringLiteral("rigctld error for command '%1': %2")
            .arg(QString::fromUtf8(m_inFlight.data.trimmed()))
            .arg(rprtLine));
    }

    switch (m_inFlight.type) {
    case CmdType::GetFreq:
        if (success && lines.size() >= 2) {
            bool ok2 = false;
            quint64 freq = lines[0].toULongLong(&ok2);
            if (ok2) {
                // Suppress jitter: emit only when delta >= 10 Hz.
                qint64 delta = static_cast<qint64>(freq)
                             - static_cast<qint64>(m_currentFreq);
                if (delta < 0) delta = -delta;
                if (delta >= 10) {
                    m_currentFreq = freq;
                    emit frequencyChanged(freq);
                }
            }
        }
        break;

    case CmdType::GetMode:
        if (success && lines.size() >= 2) {
            RadioMode mode = modeFromString(lines[0].toStdString());
            if (mode != m_currentMode) {
                m_currentMode = mode;
                emit modeChanged(mode);
            }
        }
        break;

    default:
        // Set commands (SetFreq, SetMode, SetPtt, SendMorse, StopMorse, SetVfo):
        // nothing to parse beyond success/failure already handled above.
        break;
    }
}

void RigctldRadio::handleDisconnect()
{
    m_pollTimer.stop();
    m_busy = false;
    m_queue.clear();
    m_pendingLines.clear();
    m_readBuf.clear();

    if (m_connected) {
        m_connected = false;
        emit connectionStateChanged(false);
    }

    if (!m_intentionalDisconnect && !m_reconnectTimer.isActive()) {
        m_reconnectTimer.start(m_reconnectDelay);
        m_reconnectDelay = qMin(m_reconnectDelay * 2, 30000);
    }
}

} // namespace TwoPLogger
