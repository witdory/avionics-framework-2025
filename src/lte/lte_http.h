#pragma once
#ifndef LTE_HTTP_H
#define LTE_HTTP_H

#include <ME310.h>

// 네임스페이스 명시!
bool lte_http_get(me310::ME310& modem, const char* apn, const char* server, int port);
bool lte_http_post(me310::ME310& modem, const char* apn, const char* server, int port, const char* path, const char* body);

// 명령어를 받아오는 함수 선언
bool lte_http_get_command(me310::ME310& modem, const char* apn, const char* server, int port, const char* path, char* outBuf, size_t bufSize);

#endif
