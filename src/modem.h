#pragma once
#include <ME310.h>

class Modem {
public:
    Modem();
    bool init(const char* apn);
    me310::ME310* getModem();
    

private:
    me310::ME310 _modem;
};