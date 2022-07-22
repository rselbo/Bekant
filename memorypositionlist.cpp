#include "memorypositionlist.h"

constexpr const char *ArrayName = "memory_positions";
constexpr const char *Name = "name";
constexpr const char *Position = "position";

MemoryPositionList::MemoryPositionList(QObject *parent)
    : QObject{parent}
{
    restorePositions();
    ensureDefaultItem();
}

MemoryPositionList::~MemoryPositionList()
{
    savePositions();
}

void MemoryPositionList::append(const MemoryPosition &pos)
{
    emit preMemoryPositionsAppended();
    m_data.append(pos);
    emit postMemoryPositionsAppended();
}

void MemoryPositionList::setName(qsizetype i, const QString &name)
{
    MemoryPosition &mem = m_data[i];
    if(name == mem.name) {
        return;
    }
    if(mem.position == "") {
        mem.position = "1234";
    }
    mem.name = name;
    savePositions(i);
    ensureDefaultItem();
}

void MemoryPositionList::setPosition(qsizetype i, const QString &position)
{
    MemoryPosition &mem = m_data[i];
    if(position == mem.position) {
        return;
    }
    mem.position = position;
    savePositions(i);
    ensureDefaultItem();
}

void MemoryPositionList::restorePositions()
{
    int size = m_settings.beginReadArray(ArrayName);
    for(int i = 0; i < size; ++i) {
        m_settings.setArrayIndex(i);
        QString name = m_settings.value(Name).toString();
        QString position = m_settings.value(Position).toString();
        m_data.append(MemoryPosition{name, position});
    }
    m_settings.endArray();
}

void MemoryPositionList::savePositions(int pos)
{
    m_settings.beginWriteArray(ArrayName, pos == -1 ? m_data.length() - 1 : -1);
    if (pos == -1) {
        // skip the last element
        for(int i=0; i < m_data.length() - 1; ++i) {
            m_settings.setArrayIndex(i);
            m_settings.setValue(Name, m_data.at(i).name);
            m_settings.setValue(Position, m_data.at(i).position);
        }
    }
    else {
        m_settings.setArrayIndex(pos);
        m_settings.setValue(Name, m_data.at(pos).name);
        m_settings.setValue(Position, m_data.at(pos).position);
    }
    m_settings.endArray();
}

void MemoryPositionList::ensureDefaultItem()
{
    if(m_data.empty()) {
        append(m_defaultItem);
    }
    MemoryPosition &memBack = m_data.back();
    if(memBack.position != m_defaultItem.position || memBack.name != m_defaultItem.name) {
        append(m_defaultItem);
    }
}
