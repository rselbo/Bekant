#pragma once

#include <WiFi.h>
#include <mutex>

struct cJSON;

class BekantServer
{
public:
    enum class Type : unsigned char
    {
        None,
        Debug,
        EnginePos
    };
    enum Move {
        Idle,
        Up,
        Down,
        Goto
    };
    struct QueueMessage 
    {
        Type m_type;
        char m_message[256];
    };

    BekantServer();

    static void threadFunc(void*);

    void setEnginePos(int engine1, int engine2);
    void getEnginePos(int &engine1, int &engine2);
    void setWantedPos(int wantedPos);
    int getWantedPos();
    void setMove(Move move);
    Move getMove();

    void queueMessage(Type type, const char* fmt...);
    void queueEnginePos(int pos1, int pos2);
private:
    bool handleData(WiFiClient& client, const char* buffer, unsigned short len);
    bool handlePing(WiFiClient& client, const cJSON *json);
    bool handleMove(WiFiClient& client, const cJSON *json);
    bool handleStopMove(WiFiClient& client, const cJSON *json);
    bool handleGoto(WiFiClient& client, const cJSON *json);

    bool writeMessage(WiFiClient& client, const QueueMessage &msg);

    bool writeMessageJson(WiFiClient& client, const char* type, const char* message);
    bool writeEnginePosJson(WiFiClient& client);

    QueueHandle_t m_queue;
    // read/write buffer for the networking thread
    std::unique_ptr<char> m_readbuffer;
    std::unique_ptr<char> m_writebuffer;
    std::mutex m_mutex;
    int m_Engine1 = 0;
    int m_Engine2 = 0;
    int m_wantedPos = 0;
    Move m_move = Idle;

};
