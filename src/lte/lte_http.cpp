//src/lte_http.cpp
// #include <Arduino.h>
#include <ME310.h>
#include "../modem.h"

bool lte_init(me310::ME310& modem, const char* apn) {
    int cID = 1;
    char ipProt[] = "IP";
    me310::ME310::return_t rc;

    Serial.println("------ [ME310 초기화 및 네트워크 연결] ------");
    modem.module_reboot();
    delay(10000);
    modem.powerOn(2);
    modem.report_mobile_equipment_error(2);

    modem.read_enter_pin();
    char *resp = (char *)modem.buffer_cstr(2);

    if (resp && strcmp(resp, "OK") == 0) {
        rc = modem.define_pdp_context(cID, ipProt, apn);
        Serial.print("define_pdp_context rc: ");
        Serial.println(me310::ME310::return_string(rc));

        if (rc == me310::ME310::RETURN_VALID) {
            modem.read_define_pdp_context();
            rc = modem.read_gprs_network_registration_status();
            Serial.print("network_registration_status rc: ");
            Serial.println(me310::ME310::return_string(rc));

            if (rc == me310::ME310::RETURN_VALID) {
                resp = (char *)modem.buffer_cstr(1);

                while (resp) {
                    if ((strcmp(resp, "+CGREG: 0,1") != 0) &&
                        (strcmp(resp, "+CGREG: 0,5") != 0)) {
                        delay(3000);
                        rc = modem.read_gprs_network_registration_status();
                        if (rc != me310::ME310::RETURN_VALID) break;
                        resp = (char *)modem.buffer_cstr(1);
                    } else break;
                }
            }
            modem.context_activation(cID, 1);
            return true;
        }
    } else {
        Serial.println("SIM PIN 에러 또는 연결 문제");
    }
    return false;
}

bool lte_http_get(me310::ME310& modem, const char* apn, const char* server, int port) {
    int cID = 1, connID = 1;
    char ipProt[] = "IP";
    me310::ME310::return_t rc;

    Serial.println("------ [소켓 연결/HTTP 전송] ------");
    rc = modem.socket_configuration(connID, cID);
    Serial.print("socket_configuration rc: ");
    Serial.println(me310::ME310::return_string(rc));
    if (rc != me310::ME310::RETURN_VALID) return false;

    rc = modem.socket_dial(connID, 0, port, server, 0, 0, 1, 0, 0, me310::ME310::TOUT_1MIN);
    Serial.print("socket_dial rc: ");
    Serial.println(me310::ME310::return_string(rc));
    if (rc != me310::ME310::RETURN_VALID) return false;
    delay(100);

    char http_req[128];
    snprintf(http_req, sizeof(http_req),
        "GET / HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n", server);

    rc = modem.socket_send_data_command_mode_extended(
        connID, strlen(http_req), http_req, 1, me310::ME310::TOUT_30SEC); //데이터 송신
        
    Serial.print("socket_send_data_command_mode_extended rc: ");
    Serial.println(me310::ME310::return_string(rc));

    if (rc != me310::ME310::RETURN_VALID) {
        Serial.println("HTTP GET 송신 실패");
        return false;
    }
    delay(1000);

    Serial.println("------ [응답 수신 단계] ------");
    String full_response = "";
    bool got_response = false;
    for (int i = 0; i < 1; i++) { 
        rc = modem.socket_receive_data_command_mode(connID, 512, 0, me310::ME310::TOUT_10SEC);
        Serial.println(me310::ME310::return_string(rc));
        String buf = (String)modem.buffer_cstr_raw();
        if(buf.length() > 0) {
            got_response = true;
            full_response += buf;
        }
        Serial.print("[FOR] buf.length()=");
        Serial.println(buf.length());
        Serial.print("[FOR] raw buffer: <");
        Serial.print(buf);
        Serial.println(">");
        // 수신 후 즉시 break
        break;
    }
    delay(2000);
    Serial.println("------ [최종 전체 응답 데이터] ------");
    Serial.println(full_response);
    Serial.println("[lte_http_get] return 직전");
    delay(2000);
    return got_response;

}



bool lte_http_post(me310::ME310& modem, const char* apn, const char* server, int port, 
                   const char* path, const char* body) {
    int cID = 1, connID = 1;
    char ipProt[] = "IP";
    me310::ME310::return_t rc;

    // (이전 코드와 동일: 모듈 초기화/네트워크 연결 부분 생략, 복붙해서 쓰면 됨)
    // ...
    // (이 부분은 GET 함수와 똑같이 진행)

    // ------ [소켓 연결/HTTP 전송] ------
    rc = modem.socket_configuration(connID, cID);
    if (rc != me310::ME310::RETURN_VALID) return false;

    rc = modem.socket_dial(connID, 0, port, server, 0, 0, 1, 0, 0, me310::ME310::TOUT_1MIN);
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

    rc = modem.socket_send_data_command_mode_extended(
        connID, strlen(http_req), http_req, 1, me310::ME310::TOUT_30SEC);

    if (rc != me310::ME310::RETURN_VALID) {
        Serial.println("HTTP POST 송신 실패");
        return false;
    }
    delay(1000);

    // 응답 수신도 동일
    String full_response = "";
    bool got_response = false;
    rc = modem.socket_receive_data_command_mode(connID, 512, 0, me310::ME310::TOUT_10SEC);
    String buf = (String)modem.buffer_cstr_raw();
    if(buf.length() > 0) {
        got_response = true;
        full_response += buf;
    }
    Serial.println(full_response);

    return got_response;
}

bool lte_http_get_command(me310::ME310& modem, const char* apn, const char* server, int port, const char* path, char* outBuf, size_t bufSize) {
    int cID = 1, connID = 1;
    char ipProt[] = "IP";
    me310::ME310::return_t rc;

    // 네트워크 연결 등은 기존 lte_http_get과 동일하게 처리 (생략)
    rc = modem.socket_configuration(connID, cID);
    if (rc != me310::ME310::RETURN_VALID) return false;
    rc = modem.socket_dial(connID, 0, port, server, 0, 0, 1, 0, 0, me310::ME310::TOUT_1MIN);
    if (rc != me310::ME310::RETURN_VALID) return false;
    delay(100);

    char http_req[128];
    snprintf(http_req, sizeof(http_req),
        "GET %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n", path, server);

    rc = modem.socket_send_data_command_mode_extended(
        connID, strlen(http_req), http_req, 1, me310::ME310::TOUT_30SEC);
    if (rc != me310::ME310::RETURN_VALID) return false;
    delay(1000);

    rc = modem.socket_receive_data_command_mode(connID, 256, 0, me310::ME310::TOUT_10SEC);
    String buf = (String)modem.buffer_cstr_raw();
    if(buf.length() > 0) {
        // HTTP 응답에서 명령어만 추출 (예: 마지막 줄)
        int lastCR = buf.lastIndexOf("\n");
        String cmd = buf.substring(lastCR + 1);
        cmd.trim();
        strncpy(outBuf, cmd.c_str(), bufSize-1);
        outBuf[bufSize-1] = '\0';
        return true;
    }
    return false;
}
