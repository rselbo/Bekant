#pragma once
#include <utility>
#include <stdint.h>

class BekantServer;
class Lin;

using StateReturn = std::pair<uint32_t, uint8_t>;

class BekantData 
{
public:
    enum EngineId : uint8_t { 
        Engine8 = 0x08, 
        Engine9 = 0x09 
    };
    enum State {
        Idle, // ->MovingUp, MovingDown, MovingToPos
        MovingUp, // ->StoppingMove
        MovingDown, // ->StoppingMove
        MovingToPos, // ->StoppingMove
        StoppingMove // ->Idle
    };

    BekantData(BekantServer &server, Lin &lin);

    void sendPreFrame();
    uint32_t getEnginePos(EngineId id);
    void setEnginePos(uint32_t node8Pos, uint32_t node9Pos);
    void sendCommandFrame();

private:
    inline StateReturn idleState();
    inline StateReturn stopMove(uint32_t target);
    StateReturn handleIdleState();
    StateReturn handleMovingUp();
    StateReturn handleMovingDown();
    StateReturn handleMovingToPos();
    StateReturn handleStoppingMove();

    bool atMinPos(uint32_t pos);
    bool atMaxPos(uint32_t pos);
    BekantServer &m_server;
    Lin          &m_lin;
    
    uint32_t m_node8Pos = 0;
    uint32_t m_node9Pos = 0;
    uint32_t m_targetPos = 0;
    State    m_currentState = Idle;
    uint8_t  m_stopCount = 0;
    bool     m_enginePosFailure = false;
};