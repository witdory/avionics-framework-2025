#include "HttpClient.h"
#include <Arduino.h>

int HttpClient::failCount = 0;
int HttpClient::conID = 3;

HttpClient::HttpClient(Modem* modem) : _modem(modem) {}

bool HttpClient::getCommand(
    const char* server,
    int port,
    const char* path,
    char* outBuf,
    size_t bufSize
) {
    int connID = conID + failCount;
    char ipProt[] = "IP";
    me310::ME310::return_t rc;

    Serial.println();
    Serial.println("==========================================");
    Serial.print("[GET_CMD] 요청 시작 | connID: "); Serial.print(connID);
    Serial.print(", 실패카운트: "); Serial.println(failCount);

    // 1. 소켓 설정
    rc = _modem->getModem()->socket_configuration(connID, 1);
    Serial.print("[GET_CMD][소켓설정] rc: ");
    Serial.println(me310::ME310::return_string(rc));
    if (rc != me310::ME310::RETURN_VALID) {
        Serial.println("[GET_CMD][오류] 소켓설정 실패");
        failCount++;
        _modem->getModem()->socket_listen(connID, 0, port);
        delay(300);

        if (failCount >= 5) {
            Serial.println("[GET_CMD][경고] 5회 연속 실패 - 모뎀 리셋");
            _modem->getModem()->module_reboot();
            delay(10000);
            _modem->getModem()->context_activation(1, 1);
            failCount = 0;
        }
        Serial.println("==========================================\n");
        return false;
    }

    // 2. 소켓 Dial
    rc = _modem->getModem()->socket_dial(connID, 0, port, server, 0, 0, 1, 0, 0, me310::ME310::TOUT_1MIN);
    Serial.print("[GET_CMD][소켓연결] rc: ");
    Serial.println(me310::ME310::return_string(rc));
    if (rc != me310::ME310::RETURN_VALID) {
        Serial.println("[GET_CMD][오류] 소켓연결 실패");
        failCount++;
        _modem->getModem()->socket_listen(connID, 0, port);
        delay(300);
        if (failCount >= 5) {
            Serial.println("[GET_CMD][경고] 5회 연속 실패 - 모뎀 리셋");
            _modem->getModem()->module_reboot();
            delay(10000);
            _modem->getModem()->context_activation(1, 1);
            failCount = 0;
        }
        Serial.println("==========================================\n");
        return false;
    }

    // 3. HTTP GET Request 전송
    char http_req[128];
    snprintf(http_req, sizeof(http_req),
        "GET %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n", path, server);

    Serial.println("[GET_CMD][HTTP전송] >>>");
    Serial.println(http_req);

    rc = _modem->getModem()->socket_send_data_command_mode_extended(
        connID, strlen(http_req), const_cast<char*>(http_req), 1, me310::ME310::TOUT_30SEC);
    Serial.print("[GET_CMD][데이터전송] rc: ");
    Serial.println(me310::ME310::return_string(rc));
    if (rc != me310::ME310::RETURN_VALID) {
        Serial.println("[GET_CMD][오류] 데이터 전송 실패");
        failCount++;
        _modem->getModem()->socket_listen(connID, 0, port);
        delay(300);
        Serial.println("==========================================\n");
        return false;
    }

    delay(100);

    // 4. 응답 수신
    rc = _modem->getModem()->socket_receive_data_command_mode(connID, 256, 0, me310::ME310::TOUT_10SEC);
    Serial.print("[GET_CMD][응답수신] rc: ");
    Serial.println(me310::ME310::return_string(rc));

    // 반드시 소켓 닫기 시도
    rc = _modem->getModem()->socket_listen(connID, 0, port);
    Serial.print("[GET_CMD][소켓닫기] rc: ");
    Serial.println(me310::ME310::return_string(rc));

    String buf = (String)_modem->getModem()->buffer_cstr_raw();

    Serial.println("[GET_CMD][HTTP 응답 전문 ↓↓↓]");
    Serial.println(buf);
    Serial.println("[GET_CMD][HTTP 응답 ↑↑↑]");

    if (buf.length() > 0) {
        int lastCR = buf.lastIndexOf("\n");
        String cmd = buf.substring(lastCR + 1);
        cmd.trim();
        Serial.print("[GET_CMD][명령어 파싱] : <");
        Serial.print(cmd);
        Serial.println(">");

        strncpy(outBuf, cmd.c_str(), bufSize-1);
        outBuf[bufSize-1] = '\0';

        Serial.print("[GET_CMD][OUTBUF] : <");
        Serial.print(outBuf);
        Serial.println(">");

        Serial.println("[GET_CMD][성공] 서버 명령 정상수신!");
        Serial.println("==========================================\n");
        failCount = 0; // 성공 시 실패 카운터 초기화
        return true;
    }

    failCount++;
    Serial.println("[GET_CMD][오류] 응답 없음 (파싱 실패)");
    Serial.println("==========================================\n");
    return false;
}

bool HttpClient::post(const char* server, int port, 
                   const char* path, const char* body) {
    int cID = 1, connID = 2;
    char ipProt[] = "IP";
    me310::ME310::return_t rc;

    // (이전 코드와 동일: 모듈 초기화/네트워크 연결 부분 생략, 복붙해서 쓰면 됨)
    // ...
    // (이 부분은 GET 함수와 똑같이 진행)

    // ------ [소켓 연결/HTTP 전송] ------
    rc = _modem->getModem()->socket_configuration(connID, cID);
    if (rc != me310::ME310::RETURN_VALID) return false;

    rc = _modem->getModem()->socket_dial(connID, 0, port, server, 0, 0, 1, 0, 0, me310::ME310::TOUT_1MIN);
    if (rc != me310::ME310::RETURN_VALID) return false;
    delay(100);

    // POST 패킷 생성
    int body_len = strlen(body);
    char http_req[256];
    snprintf(http_req, sizeof(http_req),
        "POST %s HTTP/1.1\r\n"
        "Host: %s\r\n"
        "Content-Type: application/x-www-form-urlencoded\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n\r\n"
        "%s", path, server, body_len, body);

    rc = _modem->getModem()->socket_send_data_command_mode_extended(
        connID, strlen(http_req), const_cast<char*>(http_req), 1, me310::ME310::TOUT_30SEC);

    if (rc != me310::ME310::RETURN_VALID) {
        Serial.println("HTTP POST 송신 실패");
        _modem->getModem()->socket_listen(connID, 0, 0);
        return false;
    }
    delay(1000);

    // 응답 수신도 동일
    String full_response = "";
    bool got_response = false;
    rc = _modem->getModem()->socket_receive_data_command_mode(connID, 512, 0, me310::ME310::TOUT_10SEC);
    String buf = (String)_modem->getModem()->buffer_cstr_raw();
    if(buf.length() > 0) {
        got_response = true;
        full_response += buf;
    }
    Serial.println(full_response);
    _modem->getModem()->socket_listen(connID, 0, 0);

    return got_response;
}
