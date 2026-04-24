#pragma once
#include "radio/RadioInterface.h"
#include <QAbstractSocket>
#include <QByteArray>
#include <QQueue>
#include <QString>
#include <QStringList>
#include <QTimer>

class QTcpSocket;

namespace TwoPLogger {

class RigctldRadio : public RadioInterface {
    Q_OBJECT
public:
    explicit RigctldRadio(const RadioProfile& profile, QObject* parent = nullptr);
    ~RigctldRadio() override;

    void connectToRigctld();
    void disconnectFromRigctld();

    // RadioInterface overrides
    void         qsy(quint64 hz)             override;
    void         setMode(RadioMode mode)     override;
    void         setPtt(bool tx)             override;
    void         sendCw(const QString& text) override;
    void         abortCw()                   override;
    bool         isConnected() const         override { return m_connected; }
    RadioProfile profile()     const         override { return m_profile; }

private slots:
    void onConnected();
    void onDisconnected();
    void onSocketError(QAbstractSocket::SocketError err);
    void onReadyRead();
    void onPollTick();
    void onReconnectTick();

private:
    enum class CmdType {
        GetFreq, GetMode,
        SetVfo, SetFreq, SetMode, SetPtt,
        SendMorse, StopMorse
    };

    struct Command {
        CmdType    type;
        QByteArray data;
    };

    void enqueueCommand(CmdType type, const QByteArray& data);
    void drain();
    void processResponse(const QStringList& lines);
    void handleDisconnect();

    RadioProfile  m_profile;
    QTcpSocket*   m_socket;
    QTimer        m_pollTimer;
    QTimer        m_reconnectTimer;

    bool          m_connected      = false;
    bool          m_busy           = false;
    bool          m_intentionalDisconnect = false;
    int           m_reconnectDelay = 1000;   // ms; doubles each failure, capped at 30 000

    quint64       m_currentFreq    = 0;
    RadioMode     m_currentMode    = RadioMode::Unknown;

    QByteArray    m_readBuf;
    QStringList   m_pendingLines;
    QQueue<Command> m_queue;
    Command       m_inFlight;
};

} // namespace TwoPLogger
