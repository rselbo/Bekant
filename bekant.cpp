#include "bekant.h"

#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QThread>
#include <QUrl>

constexpr const char *settingAddress = "address";
constexpr const char *connectedState= "connected";

Bekant::Bekant(QObject *parent)
    : QObject{parent},
      m_settings("rselbo", "bekant")
{
    connect(&m_memoryPositions, &MemoryPositionList::memoryPositionsChanged, this, &Bekant::memoryPositionsChanged);
    m_address = m_settings.value(settingAddress, "").toString();

    m_thread = std::make_unique<QThread>();
    m_thread->setObjectName("bekantNet");
    m_thread->start();
    m_thread->moveToThread(m_thread.get());
    QMetaObject::invokeMethod(m_thread.get(), [&] { initNetwork(); }, Qt::QueuedConnection);
}

Bekant::~Bekant()
{
    QThread *thread = QThread::currentThread();
    // First we delete the networking object
    QMetaObject::invokeMethod(m_network.get(), [=] { m_network.reset(); }, Qt::QueuedConnection);
    // Then we tell the thred to quit and give ownership of the m_thread back to main thread
    QMetaObject::invokeMethod(
           m_thread.get(), [=] {
               QThread::currentThread()->quit();
               m_thread->moveToThread(thread);
           },
           Qt::QueuedConnection);

    // Wait for thread to finish
    m_thread->wait();
}

/// Initialize the network in the networking thread
/// WARNING This is executed in the m_thread thread so objects belong to that thread
void Bekant::initNetwork()
{
    m_network = std::make_unique<BekantNetwork>();

    connect(m_network.get(), &BekantNetwork::connected, this, [&]() { emit connectedChanged(); });
    connect(m_network.get(), &BekantNetwork::disconnected, this, [&]() { emit connectedChanged(); });
    connect(m_network.get(), &BekantNetwork::enginePositions, this, [&](int pos1, int pos2) {
        setPosition1(pos1);
        setPosition2(pos2);
    });
    connect(m_network.get(), &BekantNetwork::pong, this, [&](const QString &pongMsg) {
        appendLine(pongMsg);
    });
    connect(this, &Bekant::connectUrl, m_network.get(), &BekantNetwork::connectUrl);
    connect(this, &Bekant::disconnectNetwork, m_network.get(), &BekantNetwork::disconnect);
    connect(this, &Bekant::sendMoveUp, m_network.get(), &BekantNetwork::sendMoveUp);
    connect(this, &Bekant::sendMoveDown, m_network.get(), &BekantNetwork::sendMoveDown);
    connect(this, &Bekant::sendMoveToPos, m_network.get(), &BekantNetwork::sendMoveToPos);
    connect(this, &Bekant::sendStopMove, m_network.get(), &BekantNetwork::sendStopMove);
    if(m_settings.value(connectedState).toBool()) {
        QUrl url(m_address);
        emit connectUrl(url);
    }
}

void Bekant::onConnect()
{
    QUrl url(m_address);
    emit connectUrl(url);
    m_settings.setValue(connectedState, true);
}

void Bekant::onDisconnect()
{
    emit disconnectNetwork();
    m_settings.setValue(connectedState, false);
}

void Bekant::upPressed()
{
    emit sendMoveUp();
}

void Bekant::upReleased()
{
    emit sendStopMove();
}

void Bekant::downPressed()
{
    emit sendMoveDown();
}

void Bekant::downReleased()
{
    emit sendStopMove();
}

void Bekant::goToPosition(int position)
{
    emit sendMoveToPos(position);
}

void Bekant::addressEdited(const QString &address)
{
    m_address = address;
    m_settings.setValue(settingAddress, m_address);
    m_settings.sync();
}

void Bekant::setAddress(const QString &address)
{
    m_address = address;
    emit addressChanged();
}

void Bekant::setText(const QString &textIn)
{
    m_text = textIn;
    emit textChanged();
}

void Bekant::appendLine(const QString &line)
{
    if(++m_lineCount > 1000) {
        m_text.remove(0, m_text.indexOf('\n')+1);
        --m_lineCount;
    }
    m_text += line + "\n";
    emit textChanged();
}

bool Bekant::isConnected()
{
    return m_network->isConnected();
}
