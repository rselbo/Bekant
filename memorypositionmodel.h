#ifndef MEMORYPOSITIONMODEL_H
#define MEMORYPOSITIONMODEL_H

#include <QAbstractListModel>
class MemoryPositionList;

class MemoryPositionModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(MemoryPositionList *list READ list WRITE setList NOTIFY listChanged)
public:
    MemoryPositionModel();

    enum {
        Name = Qt::UserRole,
        Position
    };

    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    QHash<int, QByteArray> roleNames() const override;

    MemoryPositionList *list() const;
    void setList(MemoryPositionList* list);

    Q_INVOKABLE void deleteMemoryPosition(int pos);

signals:
    void listChanged();
private:
    MemoryPositionList *m_list = nullptr;
};

#endif // MEMORYPOSITIONMODEL_H
