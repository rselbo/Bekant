#ifndef BEKANTNETWORK_H
#define BEKANTNETWORK_H

#include <QObject>
#include <QTcpSocket>
#include <QTimer>

class BekantNetwork : public QObject
{
    Q_OBJECT
public:
    explicit BekantNetwork(QObject *parent = nullptr);
    ~BekantNetwork();

    bool isConnected() { return m_socket->state() == QAbstractSocket::ConnectedState; }
public slots:
    void connectUrl(const QUrl &url);
    void disconnect();
    void sendMoveUp();
    void sendMoveDown();
    void sendStopMove();
    void sendMoveToPos(int position);
    void sendPing();

    void handleData();
signals:
    void connected();
    void disconnected();
    void enginePositions(int pos1, int pos2);
    void pong(const QString &pongMsg);

private:
    void send(const QJsonDocument &doc);

    std::unique_ptr<QTcpSocket> m_socket;
    QTimer m_pingTimer;
};

#endif // BEKANTNETWORK_H
