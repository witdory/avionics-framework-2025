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

    bool got_response = false;

    Serial.println("------ [소켓 연결/HTTP 전송] ------");
    rc = modem.socket_configuration(connID, cID);
    if (rc != me310::ME310::RETURN_VALID) return false;

    rc = modem.socket_dial(connID, 0, port, server, 0, 0, 1, 0, 0, me310::ME310::TOUT_1MIN);
    if (rc != me310::ME310::RETURN_VALID) {
        modem.socket_listen(connID, 0, port); // 실패해도 닫기
        return false;
    }
    delay(100);

    char http_req[128];
    snprintf(http_req, sizeof(http_req),
        "GET / HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n", server);

    rc = modem.socket_send_data_command_mode_extended(
        connID, strlen(http_req), http_req, 1, me310::ME310::TOUT_30SEC);

    if (rc != me310::ME310::RETURN_VALID) {
        Serial.println("HTTP GET 송신 실패");
        modem.socket_listen(connID, 0, port); // 송신 실패해도 닫기
        return false;
    }
    delay(1000);

    rc = modem.socket_receive_data_command_mode(connID, 512, 0, me310::ME310::TOUT_10SEC);
    String buf = (String)modem.buffer_cstr_raw();
    if(buf.length() > 0) {
        got_response = true;
        Serial.println(buf);
    }

    modem.socket_listen(connID, 0, port); // 반드시 닫기

    return got_response;
}



bool lte_http_post(me310::ME310& modem, const char* apn, const char* server, int port, 
                   const char* path, const char* body) {
    int cID = 1, connID = 2;
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
        modem.socket_listen(connID, 0, 0);
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
    modem.socket_listen(connID, 0, 0);

    return got_response;
}

// bool lte_http_get_command(me310::ME310& modem, const char* apn, const char* server, int port, const char* path, char* outBuf, size_t bufSize) {
//     Serial.println("[In Get Command Function]");
//     int cID = 1, connID = 1;
//     char ipProt[] = "IP";
//     me310::ME310::return_t rc;

//     // 네트워크 연결 등은 기존 lte_http_get과 동일하게 처리 (생략)
//     rc = modem.socket_configuration(connID, cID);
//     if (rc != me310::ME310::RETURN_VALID) return false;
//     rc = modem.socket_dial(connID, 0, port, server, 0, 0, 1, 0, 0, me310::ME310::TOUT_1MIN);
//     if (rc != me310::ME310::RETURN_VALID) return false;
//     delay(100);

//     char http_req[128];
//     snprintf(http_req, sizeof(http_req),
//         "GET %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n", path, server);

//     rc = modem.socket_send_data_command_mode_extended(
//         connID, strlen(http_req), http_req, 1, me310::ME310::TOUT_30SEC);
//     if (rc != me310::ME310::RETURN_VALID) return false;
//     delay(1000);

//     rc = modem.socket_receive_data_command_mode(connID, 256, 0, me310::ME310::TOUT_10SEC);
//     modem.socket_listen(connID, 0, port);

//     String buf = (String)modem.buffer_cstr_raw();
//     Serial.println("--- RAW HTTP BUF ---");
//     Serial.println(buf);
//     Serial.println("--- END BUF ---");
//     if(buf.length() > 0) {
//         // HTTP 응답에서 명령어만 추출 (예: 마지막 줄)
//         int lastCR = buf.lastIndexOf("\n");
//         String cmd = buf.substring(lastCR + 1);
//         cmd.trim();
//         strncpy(outBuf, cmd.c_str(), bufSize-1);
//         outBuf[bufSize-1] = '\0';
//         return true;
//     }
//     return false;
// }

bool lte_http_get_command(
    me310::ME310& modem,
    const char* apn,
    const char* server,
    int port,
    const char* path,
    char* outBuf,
    size_t bufSize
) {
    static int failCount = 0;
    static int conID = 3;
    int connID = conID + failCount;
    char ipProt[] = "IP";
    me310::ME310::return_t rc;

    Serial.println();
    Serial.println("==========================================");
    Serial.print("[GET_CMD] 요청 시작 | connID: "); Serial.print(connID);
    Serial.print(", 실패카운트: "); Serial.println(failCount);

    // 1. 소켓 설정
    rc = modem.socket_configuration(connID, 1);
    Serial.print("[GET_CMD][소켓설정] rc: ");
    Serial.println(me310::ME310::return_string(rc));
    if (rc != me310::ME310::RETURN_VALID) {
        Serial.println("[GET_CMD][오류] 소켓설정 실패");
        failCount++;
        modem.socket_listen(connID, 0, port);
        delay(300);

        if (failCount >= 5) {
            Serial.println("[GET_CMD][경고] 5회 연속 실패 - 모뎀 리셋");
            modem.module_reboot();
            delay(10000);
            modem.context_activation(1, 1);
            failCount = 0;
        }
        Serial.println("==========================================\n");
        return false;
    }

    // 2. 소켓 Dial
    rc = modem.socket_dial(connID, 0, port, server, 0, 0, 1, 0, 0, me310::ME310::TOUT_1MIN);
    Serial.print("[GET_CMD][소켓연결] rc: ");
    Serial.println(me310::ME310::return_string(rc));
    if (rc != me310::ME310::RETURN_VALID) {
        Serial.println("[GET_CMD][오류] 소켓연결 실패");
        failCount++;
        modem.socket_listen(connID, 0, port);
        delay(300);
        if (failCount >= 5) {
            Serial.println("[GET_CMD][경고] 5회 연속 실패 - 모뎀 리셋");
            modem.module_reboot();
            delay(10000);
            modem.context_activation(1, 1);
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

    rc = modem.socket_send_data_command_mode_extended(
        connID, strlen(http_req), http_req, 1, me310::ME310::TOUT_30SEC);
    Serial.print("[GET_CMD][데이터전송] rc: ");
    Serial.println(me310::ME310::return_string(rc));
    if (rc != me310::ME310::RETURN_VALID) {
        Serial.println("[GET_CMD][오류] 데이터 전송 실패");
        failCount++;
        modem.socket_listen(connID, 0, port);
        delay(300);
        Serial.println("==========================================\n");
        return false;
    }

    delay(100);

    // 4. 응답 수신
    rc = modem.socket_receive_data_command_mode(connID, 256, 0, me310::ME310::TOUT_10SEC);
    Serial.print("[GET_CMD][응답수신] rc: ");
    Serial.println(me310::ME310::return_string(rc));

    // 반드시 소켓 닫기 시도
    rc = modem.socket_listen(connID, 0, port);
    Serial.print("[GET_CMD][소켓닫기] rc: ");
    Serial.println(me310::ME310::return_string(rc));

    String buf = (String)modem.buffer_cstr_raw();

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
