#include "bekantnetwork.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>
#include <QThread>

BekantNetwork::BekantNetwork(QObject *parent)
    : QObject{parent}
{
    m_socket = std::make_unique<QTcpSocket>();

    connect(m_socket.get(), &QTcpSocket::connected, this, [&]() {
        emit connected();
    });
    connect(m_socket.get(), &QTcpSocket::disconnected, this, [&]() {
        emit disconnected();
    });
    connect(m_socket.get(), &QTcpSocket::stateChanged, this, [&](QAbstractSocket::SocketState socketState) {
       if(socketState != QAbstractSocket::ConnectedState) {
           emit disconnected();
       }
    });

    connect(m_socket.get(), &QIODevice::readyRead, this, &BekantNetwork::handleData);
    connect(&m_pingTimer, &QTimer::timeout, this, [&]() { sendPing(); });
    //m_pingTimer.start(5000);
}

BekantNetwork::~BekantNetwork()
{
    if(m_socket->isOpen()) {
        m_socket->close();
    }
}

void BekantNetwork::connectUrl(const QUrl &url)
{
    m_socket->connectToHost(url.host(), url.port(80));
}

void BekantNetwork::disconnect()
{
    m_socket->disconnectFromHost();
}

void BekantNetwork::sendMoveUp()
{
    QJsonObject obj = {{"type", "move"}, {"direction", "up"}};
    QJsonDocument doc(obj);
    send(doc);

}

void BekantNetwork::sendMoveDown()
{
    QJsonObject obj = {{"type", "move"}, {"direction", "down"}};
    QJsonDocument doc(obj);
    send(doc);
}

void BekantNetwork::sendStopMove()
{
    QJsonObject obj = {{"type", "stop-move"}};
    QJsonDocument doc(obj);
    send(doc);
}

void BekantNetwork::sendMoveToPos(int position)
{
    QJsonObject obj = {{"type", "goto-position"}, {"position", position}};
    QJsonDocument doc(obj);
    send(doc);
}

void BekantNetwork::sendPing()
{
    QJsonObject obj = {{"type", "ping"}};
    QJsonDocument doc(obj);
    send(doc);
}

void BekantNetwork::handleData()
{
    int bytesAvailable = m_socket->bytesAvailable();
    if(bytesAvailable < 2) {
        return;
    }
    auto lenData = m_socket->peek(2);
    quint16 len = *reinterpret_cast<quint16*>(lenData.data());
    if(bytesAvailable < (2 + len)) {
        return;
    }
    m_socket->skip(2);
    auto data = m_socket->read(len);

    auto doc = QJsonDocument::fromJson(data);
    auto obj = doc.object();

    if(obj["type"].toString() == "engine-positions") {
        emit enginePositions(obj["position1"].toInt(), obj["position2"].toInt());
    }
    if(obj["type"].toString() == "pong") {
        emit pong(obj["message"].toString());
    }
}

void BekantNetwork::send(const QJsonDocument &doc)
{
    if(!m_socket->isWritable()) {
        return;
    }
    QByteArray json = doc.toJson(QJsonDocument::Compact);
    QByteArray data;
    data.reserve(json.length() + 2);
    QDataStream stream(&data, QIODeviceBase::WriteOnly);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream << static_cast<uint16_t>(json.length());
    data.append(json);

    m_socket->write(data);
}
