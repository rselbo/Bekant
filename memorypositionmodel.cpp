#include "memorypositionmodel.h"
#include "memorypositionlist.h"

const QHash<int, QByteArray> names {{MemoryPositionModel::Name,"name"}, {MemoryPositionModel::Position, "position"}};

MemoryPositionModel::MemoryPositionModel()
{

}

int MemoryPositionModel::rowCount(const QModelIndex &parent) const
{
    if(m_list) {
        return m_list->length();
    }
    return 0;
}

QVariant MemoryPositionModel::data(const QModelIndex &index, int role) const
{
    if (!m_list) {
        return QVariant();
    }
    switch(role){
    case MemoryPositionModel::Name:
        if (m_list->length() < index.row()) {
            return QVariant();
        }
        return m_list->at(index.row()).name;
    case MemoryPositionModel::Position:
        if (m_list->length() < index.row()) {
            return QVariant();
        }
        return m_list->at(index.row()).position;
    }
    return QVariant();
}

bool MemoryPositionModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!m_list) {
        return false;
    }
    switch(role){
    case MemoryPositionModel::Name:
        if (m_list->length() < index.row()) {
            return false;
        }
        m_list->setName(index.row(), value.toString());
        emit dataChanged(index,index);
        return true;
    case MemoryPositionModel::Position:
        if (m_list->length() < index.row()) {
            return false;
        }
        m_list->setPosition(index.row(), value.toString());
        emit dataChanged(index,index);
    }
    return false;
}

QHash<int, QByteArray> MemoryPositionModel::roleNames() const
{
    return names;
}

MemoryPositionList *MemoryPositionModel::list() const
{
    return m_list;
}

void MemoryPositionModel::setList(MemoryPositionList* list)
{
    beginResetModel();
    if(m_list) {
        m_list->disconnect(this);
    }
    m_list = list;

    connect(m_list, &MemoryPositionList::preMemoryPositionsAppended, this, [this](){
        qsizetype index = m_list->length();
        beginInsertRows(QModelIndex(), index, index);
    });
    connect(m_list, &MemoryPositionList::postMemoryPositionsAppended, this, [this](){
        endInsertRows();
    });
}

void MemoryPositionModel::deleteMemoryPosition(int pos)
{
    beginRemoveRows(QModelIndex(), pos, pos);
    m_list->removeAt(pos);
    endRemoveRows();
}
