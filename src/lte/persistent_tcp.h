#pragma once
#include "../modem.h"

class PersistentTCP {
public:
    PersistentTCP(Modem* modem, const char* server_ip, int port);
    bool connect();
    bool send(const char* data);
    void close();
    bool isConnected();
    String receive();

private:
    Modem* _modem;
    int _connID;
    int _port;
    const char* _server_ip;
    bool _connected;
};