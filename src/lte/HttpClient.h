#pragma once
#include "../modem.h"

class HttpClient {
public:
    HttpClient(Modem* modem);
    bool getCommand(const char* server, int port, const char* path, char* outBuf, size_t bufSize);
    bool post(const char* server, int port, const char* path, const char* body);

private:
    Modem* _modem;
    static int failCount;
    static int conID;
};
