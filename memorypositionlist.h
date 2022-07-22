#ifndef MEMORYPOSITION_H
#define MEMORYPOSITION_H

#include <QObject>
#include <QList>
#include <QSettings>

struct MemoryPosition {
    QString name;
    QString position;
};

class MemoryPositionList : public QObject
{
    Q_OBJECT
public:
    explicit MemoryPositionList(QObject *parent = nullptr);
    virtual ~MemoryPositionList();

    void append(const MemoryPosition &pos);
    qsizetype length() const { return m_data.length(); }
    MemoryPosition at(qsizetype i) const { return m_data.at(i); }
    QList<MemoryPosition>::reference operator[](qsizetype i) { return m_data[i]; }
    void removeAt(qsizetype i) { m_data.removeAt(i); savePositions(); }

    void setName(qsizetype i, const QString &name);
    void setPosition(qsizetype i, const QString &position);
signals:
    void memoryPositionsChanged();
    void preMemoryPositionsAppended();
    void postMemoryPositionsAppended();
private:
    void restorePositions();
    void savePositions(int pos = -1);
    void ensureDefaultItem();
    QSettings m_settings;
    QList<MemoryPosition> m_data;
    const MemoryPosition m_defaultItem{"Enter name", ""};
};

#endif // MEMORYPOSITION_H
