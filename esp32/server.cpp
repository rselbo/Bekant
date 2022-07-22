#include "server.h"
#include "cJSON.h"
#include <sys/socket.h>
#include <vector>

constexpr const int BUFFER_SIZE = 2048;

#define Strings(name, str) \
constexpr const char *name = str; \
constexpr int name##Len = strlen(name)

// incoming commands
Strings(cmdPing, "ping");
Strings(cmdMove, "move");
Strings(cmdStopMove, "stop-move");
Strings(cmdGoto, "goto-position");

// outcoing commands
Strings(cmdPong, "pong");
Strings(cmdDebug, "debug");
Strings(cmdEnginePositions, "engine-positions");

// json field names
Strings(fieldType, "type");
Strings(fieldMessage, "message");
Strings(fieldPosition, "position");
Strings(fieldPosition1, "position1");
Strings(fieldPosition2, "position2");
Strings(fieldDirection, "direction");

Strings(directionUp, "up");
Strings(directionDown, "down");

void BekantServer::threadFunc(void* context)
{
    BekantServer &bekantServer = *reinterpret_cast<BekantServer*>(context);

    WiFiServer server(80);
    server.begin();

    struct ClientWithLen {
        WiFiClient client;
        unsigned short expectedLen;
    };
    std::vector<ClientWithLen> clients;
    for(;;) {
        bool shouldSleep = true;

        WiFiClient client = server.available();

        // Check if we have a new client and add it to the list
        if(client && client.connected()) {
            clients.push_back({client, 0});
            bekantServer.writeEnginePosJson(client);
        }

        QueueMessage msg{BekantServer::Type::None};
        xQueueReceive(bekantServer.m_queue, &msg, 0);

        for(auto &&i = clients.begin(); i != clients.end();) {
            WiFiClient &client = i->client;
            unsigned short &len = i ->expectedLen;

            // if the client is not connected, remove it from our list
            if (!client.connected()) {
                i = clients.erase(i);
                continue;
            }

            // if we have not read the length, try to read the length
            if (len == 0 && client.available() >= 2) {
                client.readBytes(reinterpret_cast<char*>(&len), 2);
                // data length bailout
                if(len > 2048) {
                    client.stop();
                    continue;
                }
            }
            
            if(len > 0 && client.available() >= len) {
                client.readBytes(bekantServer.m_readbuffer.get(), len);
                bekantServer.m_readbuffer.get()[len] = 0;
                if(!bekantServer.handleData(client, bekantServer.m_readbuffer.get(), len)) {
                    client.stop();
                    len = 0;
                    continue;
                }
                len = 0;
                shouldSleep = false;
            }

            if(msg.m_type != BekantServer::Type::None) {
                bekantServer.writeMessage(client, msg);
            }
            ++i;
        }

        if(shouldSleep) {
            sys_msleep(100);
        }
    }
}

BekantServer::BekantServer() : 
    m_readbuffer(new char[BUFFER_SIZE]),
    m_writebuffer(new char[BUFFER_SIZE])
{
    m_queue = xQueueCreate( 10, sizeof( QueueMessage ) );
}

void BekantServer::setEnginePos(int engine1, int engine2)
{
    std::lock_guard<std::mutex> lck(m_mutex);
    m_Engine1 = engine1;
    m_Engine2 = engine2;
}

void BekantServer::getEnginePos(int &engine1, int &engine2)
{
    std::lock_guard<std::mutex> lck(m_mutex);
    engine1 = m_Engine1;
    engine2 = m_Engine2;
}

void BekantServer::setWantedPos(int wantedPos)
{
    std::lock_guard<std::mutex> lck(m_mutex);
    m_wantedPos = wantedPos;
}

int BekantServer::getWantedPos()
{
    std::lock_guard<std::mutex> lck(m_mutex);
    return m_wantedPos;
}

void BekantServer::setMove(Move move)
{
    std::lock_guard<std::mutex> lck(m_mutex);
    m_move = move;
}

BekantServer::Move BekantServer::getMove()
{
    std::lock_guard<std::mutex> lck(m_mutex);
    return m_move;
}

bool BekantServer::handleData(WiFiClient& client, const char* buffer, unsigned short len)
{
    Serial.println(buffer);
    bool ret = true;
    cJSON *json = cJSON_ParseWithLength(buffer, len);
    if(json == nullptr || cJSON_IsInvalid(json)) {
        cJSON_Delete(json);
        return false;
    }
    if(!cJSON_IsObject(json)) {
        cJSON_Delete(json);
        return false;
    }
    cJSON *type = cJSON_GetObjectItemCaseSensitive(json, fieldType);
    if(type == nullptr || !cJSON_IsString(type)) {
        cJSON_Delete(json);
        return false; 
    }

    if(strncmp(cJSON_GetStringValue(type), cmdPing, cmdPingLen) == 0) {
        ret = handlePing(client, json);
    }    
    else if(strncmp(cJSON_GetStringValue(type), cmdMove, cmdMoveLen) == 0) {
        ret = handleMove(client, json);
    }
    else if(strncmp(cJSON_GetStringValue(type), cmdStopMove, cmdStopMoveLen) == 0) {
        ret = handleStopMove(client, json);
    }
    else if(strncmp(cJSON_GetStringValue(type), cmdGoto, cmdGotoLen) == 0) {
        ret = handleGoto(client, json);
    }

    // cleanup
    cJSON_Delete(json);
    return ret;
}

bool BekantServer::handlePing(WiFiClient& client, const cJSON *json)
{
    char buffer[20];
    snprintf(buffer, 20, "free mem %d", esp_get_free_heap_size());
    return writeMessageJson(client, cmdPong, buffer);
}

bool BekantServer::handleMove(WiFiClient& client, const cJSON *json)
{
    cJSON *direction = cJSON_GetObjectItemCaseSensitive(json, fieldDirection);
    if(direction == nullptr || !cJSON_IsString(direction)) {
        return false;
    }

    const char* dir = cJSON_GetStringValue(direction);
    if(dir == nullptr) {
        return false;
    }

    if(strncmp(dir, directionUp, directionUpLen) == 0) {
        setMove(Up);
    }
    else if(strncmp(dir, directionDown, directionDownLen) == 0) {
        setMove(Down);
    }
    else {
        return false;
    }
    return true;
}

bool BekantServer::handleStopMove(WiFiClient& client, const cJSON *json)
{
    setMove(Idle);
    return true;
}

bool BekantServer::handleGoto(WiFiClient& client, const cJSON *json)
{
    cJSON *position = cJSON_GetObjectItemCaseSensitive(json, fieldPosition);
    if(position == nullptr || !cJSON_IsNumber(position)) {
        return false;
    }

    double pos = cJSON_GetNumberValue(position);
    if(pos == NAN) {
        return false;
    }
    setWantedPos(pos);
    setMove(Goto);
    return true;
}

void BekantServer::queueMessage(Type type, const char* fmt...)
{
    QueueMessage msg{type};
    va_list args;
    va_start(args, fmt);
    int print = vsnprintf(msg.m_message, sizeof(QueueMessage::m_message), fmt, args);
    va_end(args);
    if(print < 0){
        return;
    }

    xQueueSend(m_queue, &msg, 0);
}

void BekantServer::queueEnginePos(int pos1, int pos2)
{
    QueueMessage msg{Type::EnginePos};

    xQueueSend(m_queue, &msg, 0);
}

bool BekantServer::writeMessage(WiFiClient& client, const QueueMessage &msg)
{
    switch(msg.m_type){
        case Type::Debug:
            return writeMessageJson(client, cmdDebug, msg.m_message);
        case Type::EnginePos:
            return writeEnginePosJson(client);
        default:
            client.stop();
            return false;
    }

    return true;
}

bool BekantServer::writeMessageJson(WiFiClient& client, const char* type, const char* message)
{
    // craft reply
    cJSON *returnMessage = cJSON_CreateObject();
    cJSON_AddItemToObject(returnMessage, fieldType, cJSON_CreateStringReference(type));
    cJSON_AddItemToObject(returnMessage, fieldMessage, cJSON_CreateStringReference(message));
    cJSON_bool print = cJSON_PrintPreallocated(returnMessage, m_writebuffer.get()+2, BUFFER_SIZE-2, false);
    cJSON_Delete(returnMessage);
    
    if(!print) {
        Serial.println("json serializing failed");
        return false;
    }
    
    size_t msgLen = strlen(m_writebuffer.get()+2);
    *reinterpret_cast<unsigned short*>(m_writebuffer.get()) = static_cast<unsigned short>(msgLen);
    size_t written = client.write(m_writebuffer.get(), msgLen+2);
    return written == (msgLen + 2);
}

bool BekantServer::writeEnginePosJson(WiFiClient& client)
{
    int engine1 = 0;
    int engine2 = 0;
    getEnginePos(engine1, engine2);
    Serial.println("sending engine pos");
    // craft reply
    cJSON *returnMessage = cJSON_CreateObject();
    cJSON_AddItemToObject(returnMessage, fieldType, cJSON_CreateStringReference(cmdEnginePositions));
    cJSON_AddItemToObject(returnMessage, fieldPosition1, cJSON_CreateNumber(engine1));
    cJSON_AddItemToObject(returnMessage, fieldPosition2, cJSON_CreateNumber(engine2));
    cJSON_bool print = cJSON_PrintPreallocated(returnMessage, m_writebuffer.get()+2, BUFFER_SIZE-2, false);
    cJSON_Delete(returnMessage);
    
    if(!print) {
        Serial.println("json serializing failed");
        return false;
    }
    
    size_t msgLen = strlen(m_writebuffer.get()+2);
    *reinterpret_cast<unsigned short*>(m_writebuffer.get()) = static_cast<unsigned short>(msgLen);
    size_t written = client.write(m_writebuffer.get(), msgLen+2);
    return written == (msgLen + 2);
}