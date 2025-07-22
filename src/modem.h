#pragma once
#include <ME310.h>

extern me310::ME310 modem;

bool lte_init(me310::ME310& modem, const char* apn);