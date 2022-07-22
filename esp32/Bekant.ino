#include <WiFi.h>
#include "server.h"
#include "lin.h"

const char* ssid     = "";
const char* password = "";

#define ONBOARD_LED  2
constexpr uint8_t empty[3] = {0,0,0};

#define LIN_MOTOR_BUSY_FINE     0x03
#define LIN_MOTOR_BUSY          0x02
#define LIN_CMD_FINISH          0x84
#define LIN_CMD_LOWER           0x85
#define LIN_CMD_RAISE           0x86
#define LIN_CMD_FINE            0x87
#define LIN_CMD_RECALIBRATE_END 0xbc
#define LIN_CMD_RECALIBRATE     0xbd
#define LIN_CMD_PREMOVE         0xc4
#define LIN_CMD_IDLE            0xfc

#define FINE_TARGET 110
#define MINIMUM_POS 177
#define MAXIMUM_POS 6360

// These are the 3 known idle states
#define LIN_MOTOR_IDLE1 0x00
#define LIN_MOTOR_IDLE2 0x25
#define LIN_MOTOR_IDLE3 0x60

BekantServer server;
Lin lin;
uint32_t node8Pos = 0;
uint32_t node9Pos = 0;
uint32_t targetPos = 0;
uint8_t  stopCount = 0;

enum State {
  Idle, // ->MovingUp, MovingDown
  MovingUp, // ->StoppingMove
  MovingDown, // ->StoppingMove
  MovingToPos, // ->StoppingMove
  StoppingMove // ->Idle
} currentState;

void setup()
{
    Serial.begin(115200);

    WiFi.begin(ssid, password);
    WiFi.setAutoReconnect(true);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
    }

    lin.setup();
   
    // Create the linbus handler
    TaskHandle_t handle;
    //xTaskCreate(&BekantServer::threadFunc, "serverFunc", 4000, &server, 0, &handle);
    if(xTaskCreatePinnedToCore(&BekantServer::threadFunc, "serverFunc", 4000, &server, 1, &handle, 0) != pdPASS)
    {
      Serial.printf("server task failed\n");
    }

    Serial.printf("starting on %d\n", xPortGetCoreID());
    pinMode(ONBOARD_LED,OUTPUT);
    digitalWrite(ONBOARD_LED, HIGH);
    currentState = State::Idle;
}

bool atMinPos(uint32_t pos) {
  return pos < (MINIMUM_POS + FINE_TARGET);
}

bool atMaxPos(uint32_t pos) {
  return pos > (MAXIMUM_POS - FINE_TARGET);
}

void verifyWifi() 
{
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(ONBOARD_LED, LOW);
    delay(500);
  }
  digitalWrite(ONBOARD_LED, HIGH);
}

void loop(){
  verifyWifi();

  uint8_t buffer[8];
  int recvLen = 0;
  
  lin.writeFrame(0x11, empty, 3);
  usleep(500);

  uint32_t node8 = 0;
  uint32_t node9 = 0;
  memset(buffer, 0, 8);
  recvLen = lin.receiveFrame(0x8, buffer, 3);
  if(recvLen < 0) {
    server.queueMessage(BekantServer::Type::Debug, "receiveFrame error 0x08 %d %x %x %x", recvLen, buffer[0], buffer[1], buffer[2]);
  } else {
    node8 = buffer[0] | (buffer[1] << 8);
  }
  usleep(500);

  memset(buffer, 0, 8);
  recvLen = lin.receiveFrame(0x9, buffer, 3);
  if(recvLen < 0) {
    server.queueMessage(BekantServer::Type::Debug, "receiveFrame error 0x09 %d %x %x %x", recvLen, buffer[0], buffer[1], buffer[2]);
  } else {
    node9 = buffer[0] | (buffer[1] << 8);
  }

  if(node8 != node8Pos || node9 != node9Pos) {
    node8Pos = node8;
    node9Pos = node9;
    server.setEnginePos(node8Pos, node9Pos);
    server.queueEnginePos(node8, node9);
  }
  usleep(500);

  uint32_t minPos = std::min(node8Pos, node9Pos);
  uint32_t maxPos = std::max(node8Pos, node9Pos);
  uint32_t encodePos = minPos;
  uint8_t command = LIN_CMD_IDLE;

  switch(currentState) {
    case State::Idle:
      switch(server.getMove())
      {
        case BekantServer::Move::Up:
          if(atMaxPos(minPos)) {
            //we are at highest pos, do not try to go further
            break;
          }
          command = LIN_CMD_PREMOVE;
          // State Idle -> MovingUp
          currentState = MovingUp;
          break;
        case BekantServer::Move::Down:
          if(atMinPos(maxPos)) {
            //we are at lowest pos, do not try to go further
            break;
          }
          command = LIN_CMD_PREMOVE;
          // State Idle -> MovingDown
          currentState = MovingDown;
          break;
        case BekantServer::Move::Goto:
          command = LIN_CMD_PREMOVE;
          // State Idle -> MovingGoto
          currentState = MovingToPos;
          break;
      }
      break;
    case State::MovingUp:
      if(atMaxPos(minPos)) {
        // force stop
        server.setMove(BekantServer::Move::Idle);
      }
      if(server.getMove() != BekantServer::Move::Up) {
        // State MovingUp -> StoppingMove
        currentState = StoppingMove;
        targetPos = minPos + FINE_TARGET;
        encodePos = targetPos;
        command = LIN_CMD_FINE;
        stopCount = 0;
      } else {
        encodePos = minPos;
        command = LIN_CMD_RAISE;
      }
      break;
    case State::MovingDown:
      if(atMinPos(maxPos)) {
        //force stop
        server.setMove(BekantServer::Move::Idle);
      }

      if(server.getMove() != BekantServer::Move::Down) {
        // State MovingDown -> StoppingMove
        currentState = StoppingMove;
        targetPos = maxPos - FINE_TARGET;
        encodePos = targetPos;
        command = LIN_CMD_FINE;
        stopCount = 0;
      } else {
        encodePos = maxPos;
        command = LIN_CMD_LOWER;
      }
      break;
    case State::MovingToPos:
    {
      bool movingUp = server.getWantedPos() > encodePos;
      if(movingUp) {
        if(atMaxPos(minPos)) {
          server.setMove(BekantServer::Move::Idle);
        }
        if(minPos + FINE_TARGET >= server.getWantedPos()) {
          server.setMove(BekantServer::Move::Idle);
        }
        encodePos = minPos;
        command = LIN_CMD_RAISE;
      } else {
        if(atMinPos(maxPos)) {
          server.setMove(BekantServer::Move::Idle);
        }
        if(maxPos - FINE_TARGET <= server.getWantedPos()) {
          server.setMove(BekantServer::Move::Idle);
        }
        encodePos = maxPos;
        command = LIN_CMD_LOWER;
      }

      if(server.getMove() != BekantServer::Move::Goto){
        // State MovingDown -> StoppingMove
        currentState = StoppingMove;
        targetPos = movingUp ? minPos + FINE_TARGET : maxPos - FINE_TARGET;
        encodePos = targetPos;
        command = LIN_CMD_FINE;
        stopCount = 0;
      }

      break;
    }
    case State::StoppingMove:
      if(stopCount++ >= 3) {
        command = LIN_CMD_FINISH;
        // State StoppingMove -> Idle
        currentState = Idle;
      } else {
        encodePos = targetPos;
        command = LIN_CMD_FINE;
      }
      break;
  }
  buffer[0] = encodePos & 0xff;
  buffer[1] = (encodePos >> 8) & 0xff;
  buffer[2] = command;

  lin.writeFrame(0x12, buffer, 3);

  usleep(150000);
}

