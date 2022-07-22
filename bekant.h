#ifndef BEKANT_H
#define BEKANT_H

#include "memorypositionlist.h"
#include "bekantnetwork.h"

#include <QObject>
#include <QSettings>
#include <QTcpSocket>
#include <QTimer>

class QThread;

class Bekant : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString address READ address WRITE setAddress NOTIFY addressChanged)
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    Q_PROPERTY(bool connected READ isConnected NOTIFY connectedChanged)
    Q_PROPERTY(MemoryPositionList *memoryPositions READ memoryPositions NOTIFY memoryPositionsChanged )
    Q_PROPERTY(int position READ position NOTIFY positionChanged)
    Q_PROPERTY(int position1 READ position1 NOTIFY positionChanged)
    Q_PROPERTY(int position2 READ position2 NOTIFY positionChanged)
public:
    explicit Bekant(QObject *parent = nullptr);
    ~Bekant();

    void initNetwork();

    /// Q_PROPERTY address functions
    QString address() const { return m_address; }
    void setAddress(const QString& address);
    /// Q_PROPERTY text functions
    QString text() const { return m_text; }
    void setText(const QString &textIn);
    /// Q_PROPERTY connected functions
    bool isConnected();
    /// Q_PROPERTY memoryPositions functions
    MemoryPositionList *memoryPositions() { return &m_memoryPositions; }
    /// Q_PROPERTY position functions
    int position() const { return (m_position1 + m_position2) / 2; }
    /// Q_PROPERTY position1 functions
    int position1() const { return m_position1; }
    void setPosition1(int position1) { m_position1 = position1; emit positionChanged(); }
    /// Q_PROPERTY position2 functions
    int position2() const { return m_position2; }
    void setPosition2(int position2) { m_position2 = position2; emit positionChanged(); }

    void appendLine(const QString &line);

public slots:
    void onConnect();
    void onDisconnect();

    void upPressed();
    void upReleased();
    void downPressed();
    void downReleased();

    void goToPosition(int position);

    void addressEdited(const QString &address);
signals:
    void addressChanged();
    void textChanged();
    void connectedChanged();
    void memoryPositionsChanged();
    void positionChanged();

    void connectUrl(const QUrl &url);
    void disconnectNetwork();
    void sendMoveUp();
    void sendMoveDown();
    void sendMoveToPos(int position);
    void sendStopMove();


private:
    QString m_text;
    int m_lineCount = 0;
    MemoryPositionList m_memoryPositions;
    int m_position1 = 0;
    int m_position2 = 0;
    QString m_address;
    std::unique_ptr<QThread> m_thread;
    std::unique_ptr<BekantNetwork> m_network;

    QSettings m_settings;
};

#endif // BEKANT_H
