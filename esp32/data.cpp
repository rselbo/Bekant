#include "data.h"
#include "server.h"
#include "lin.h"

constexpr const uint8_t empty[3] = {0, 0, 0};

// Bekant specific command ids
constexpr const uint8_t LIN_MOTOR_BUSY_FINE = 0x03;
constexpr const uint8_t LIN_MOTOR_BUSY = 0x02;
constexpr const uint8_t LIN_CMD_FINISH = 0x84;
constexpr const uint8_t LIN_CMD_LOWER = 0x85;
constexpr const uint8_t LIN_CMD_RAISE = 0x86;
constexpr const uint8_t LIN_CMD_FINE = 0x87;
constexpr const uint8_t LIN_CMD_RECALIBRATE_END = 0xbc;
constexpr const uint8_t LIN_CMD_RECALIBRATE = 0xbd;
constexpr const uint8_t LIN_CMD_PREMOVE = 0xc4;
constexpr const uint8_t LIN_CMD_IDLE = 0xfc;

// some constant values that is specific to my desk
constexpr const uint8_t FINE_TARGET = 110;
constexpr const uint16_t MINIMUM_POS = 177;
constexpr const uint16_t MAXIMUM_POS = 6360;

// These are the 3 known idle states that the bekant engines report
constexpr const uint8_t LIN_MOTOR_IDLE1 = 0x00;
constexpr const uint8_t LIN_MOTOR_IDLE2 = 0x25;
constexpr const uint8_t LIN_MOTOR_IDLE3 = 0x60;

BekantData::BekantData(BekantServer &server, Lin &lin) : m_server(server),
                                                         m_lin(lin)
{
}

void BekantData::sendPreFrame()
{
    m_lin.writeFrame(0x11, empty, 3);
    // reset eny previous engine pos failures
    m_enginePosFailure = false;
}

uint32_t BekantData::getEnginePos(EngineId id)
{
    uint8_t buffer[3] = {0, 0, 0};
    int recvLen = m_lin.receiveFrame(id, buffer, 3);
    if (recvLen < 0)
    {
        m_server.queueMessage(BekantServer::Type::Debug, "receiveFrame error 0x08 %d %x %x %x", recvLen, buffer[0], buffer[1], buffer[2]);
        m_enginePosFailure = true;
        return 0;
    }

    return (buffer[0] | (buffer[1] << 8));
}

void BekantData::setEnginePos(uint32_t node8Pos, uint32_t node9Pos)
{
    if(m_enginePosFailure) {
        return;        
    }

    if (m_node8Pos != node8Pos || m_node9Pos != node9Pos) {
        m_node8Pos = node8Pos;
        m_node9Pos = node9Pos;
        m_server.setEnginePos(m_node8Pos, m_node9Pos);
        m_server.queueEnginePos(m_node8Pos, m_node9Pos);

    }
}

void BekantData::sendCommandFrame()
{
    uint32_t encodePos = 0;
    uint8_t command = LIN_CMD_IDLE;

    switch (m_currentState)
    {
    case State::Idle:
        std::tie(encodePos, command) = handleIdleState();
        break;
    case State::MovingUp:
        std::tie(encodePos, command) = handleMovingUp();
        break;
    case State::MovingDown:
        std::tie(encodePos, command) = handleMovingDown();
        break;
    case State::MovingToPos:
        std::tie(encodePos, command) = handleMovingToPos();
        break;
    case State::StoppingMove:
        std::tie(encodePos, command) = handleStoppingMove();
        break;
    }
    uint8_t buffer[3] = {
        static_cast<uint8_t>(encodePos & 0xff),
        static_cast<uint8_t>((encodePos >> 8) & 0xff),
        command};

    m_lin.writeFrame(0x12, buffer, 3);
}

StateReturn BekantData::idleState()
{
    return std::make_pair(std::min(m_node8Pos, m_node9Pos), LIN_CMD_IDLE);
}

StateReturn BekantData::stopMove(uint32_t targetPos)
{
    // State MovingUp/Down/ToPos -> StoppingMove
    m_currentState = StoppingMove;
    m_targetPos = targetPos;
    m_stopCount = 0;
    return std::make_pair(m_targetPos, LIN_CMD_FINE);
}

StateReturn BekantData::handleIdleState()
{
    uint32_t minPos = std::min(m_node8Pos, m_node9Pos);
    uint32_t maxPos = std::max(m_node8Pos, m_node9Pos);

    switch (m_server.getMove())
    {
    case BekantServer::Move::Up:
        if (atMaxPos(minPos))
        {
            // we are at highest pos, do not try to go further
            break;
        }
        // State Idle -> MovingUp
        m_currentState = MovingUp;
        return std::make_pair(minPos, LIN_CMD_PREMOVE);
    case BekantServer::Move::Down:
        if (atMinPos(maxPos))
        {
            // we are at lowest pos, do not try to go further
            break;
        }
        // State Idle -> MovingDown
        m_currentState = MovingDown;
        return std::make_pair(minPos, LIN_CMD_PREMOVE);
    case BekantServer::Move::Goto:
        // State Idle -> MovingGoto
        m_currentState = MovingToPos;
        return std::make_pair(minPos, LIN_CMD_PREMOVE);
    }
    return idleState();
}

StateReturn BekantData::handleMovingUp()
{
    uint32_t minPos = std::min(m_node8Pos, m_node9Pos);
    if (atMaxPos(minPos))
    {
        // force stop, and fall through the rest of the function so we can stop properly
        m_server.setMove(BekantServer::Move::Idle);
    }

    if (m_server.getMove() != BekantServer::Move::Up)
    {
        return stopMove(minPos + FINE_TARGET);
    }

    return std::make_pair(minPos, LIN_CMD_RAISE);
}

StateReturn BekantData::handleMovingDown()
{
    uint32_t maxPos = std::max(m_node8Pos, m_node9Pos);
    if (atMinPos(maxPos))
    {
        // force stop, and fall through the rest of the function so we can stop properly
        m_server.setMove(BekantServer::Move::Idle);
    }

    if (m_server.getMove() != BekantServer::Move::Down)
    {
        return stopMove(maxPos - FINE_TARGET);
    }

    return std::make_pair(maxPos, LIN_CMD_LOWER);
}

StateReturn BekantData::handleMovingToPos()
{
    uint32_t minPos = std::min(m_node8Pos, m_node9Pos);
    uint32_t maxPos = std::max(m_node8Pos, m_node9Pos);
    bool movingUp = m_server.getWantedPos() > minPos;

    if (m_server.getMove() != BekantServer::Move::Goto)
    {
        return stopMove(movingUp ? minPos + FINE_TARGET : maxPos - FINE_TARGET);
    }

    if (movingUp)
    {
        if (atMaxPos(minPos))
        {
            m_server.setMove(BekantServer::Move::Idle);
            return stopMove(minPos + FINE_TARGET);
        }
        if (minPos + FINE_TARGET >= m_server.getWantedPos())
        {
            m_server.setMove(BekantServer::Move::Idle);
            return stopMove(minPos + FINE_TARGET);
        }
        return std::make_pair(minPos, LIN_CMD_RAISE);
    }
    else
    {
        if (atMinPos(maxPos))
        {
            m_server.setMove(BekantServer::Move::Idle);
            return stopMove(maxPos - FINE_TARGET);
        }
        if (maxPos - FINE_TARGET <= m_server.getWantedPos())
        {
            m_server.setMove(BekantServer::Move::Idle);
            return stopMove(maxPos - FINE_TARGET);
        }
        return std::make_pair(maxPos, LIN_CMD_LOWER);
    }
}

StateReturn BekantData::handleStoppingMove()
{
    uint32_t minPos = std::min(m_node8Pos, m_node9Pos);
    if (m_stopCount++ >= 3)
    {
        // State StoppingMove -> Idle
        m_currentState = Idle;
        return std::make_pair(minPos, LIN_CMD_FINISH);
    }
    else
    {
        return std::make_pair(m_targetPos, LIN_CMD_FINE);
    }
}

bool BekantData::atMinPos(uint32_t pos)
{
    return pos < (MINIMUM_POS + FINE_TARGET);
}

bool BekantData::atMaxPos(uint32_t pos)
{
    return pos > (MAXIMUM_POS - FINE_TARGET);
}
