#include "modem.h"
#include <Arduino.h>

Modem::Modem() {}

bool Modem::init(const char* apn) {
    int cID = 1;
    char ipProt[] = "IP";
    me310::ME310::return_t rc;

    Serial.println("------ [ME310 초기화 및 네트워크 연결] ------");
    _modem.module_reboot();
    delay(5000);
    _modem.powerOn(2);
    _modem.report_mobile_equipment_error(2);

    _modem.read_enter_pin();
    char *resp = (char *)_modem.buffer_cstr(2);

    if (resp && strcmp(resp, "OK") == 0) {
        rc = _modem.define_pdp_context(cID, ipProt, apn);
        Serial.print("define_pdp_context rc: ");
        Serial.println(me310::ME310::return_string(rc));

        if (rc == me310::ME310::RETURN_VALID) {
            _modem.read_define_pdp_context();
            rc = _modem.read_gprs_network_registration_status();
            Serial.print("network_registration_status rc: ");
            Serial.println(me310::ME310::return_string(rc));

            if (rc == me310::ME310::RETURN_VALID) {
                resp = (char *)_modem.buffer_cstr(1);

                while (resp) {
                    if ((strcmp(resp, "+CGREG: 0,1") != 0) &&
                        (strcmp(resp, "+CGREG: 0,5") != 0)) {
                        delay(3000);
                        rc = _modem.read_gprs_network_registration_status();
                        if (rc != me310::ME310::RETURN_VALID) break;
                        resp = (char *)_modem.buffer_cstr(1);
                    } else break;
                }
            }
            _modem.context_activation(cID, 1);
            return true;
        }
    } else {
        Serial.println("SIM PIN 에러 또는 연결 문제");
    }
    return false;
}

me310::ME310* Modem::getModem() {
    return &_modem;
}


