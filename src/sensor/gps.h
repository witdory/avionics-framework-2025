#pragma once
#include <Arduino.h>
#include <ME310.h>
#include "../lib/sensor.h"

#define ON_OFF 2 // GNSS 모듈 ON/OFF 제어 핀 (필요에 따라 수정)
#define MDMSerial Serial1 // ME310 모듈과 연결된 시리얼 포트

using namespace me310;

class GPS : public Sensor<float> {
public:
    GPS(int8_t sensorMode, uint8_t datalength, ME310* modem)
        : Sensor<float>(sensorMode, datalength), _modem(modem), lastTime(0) {}

    void init() {
        MDMSerial.begin(115200);
        delay(100);
        _modem->debugMode(false);
        _modem->module_reboot();
        delay(5000);
        _modem->powerOn(ON_OFF);

        _modem->report_mobile_equipment_error(2);
        _modem->read_gnss_configuration();
        _modem->gnss_configuration(0, 0); // GNSS 우선
        _modem->gnss_configuration(2, 1); // GPS+GLO
        _modem->gnss_configuration(3, 0); // 런타임 GNSS 우선
        _modem->gnss_controller_power_management(0); // GNSS 컨트롤러 OFF

        lastTime = millis();
        _data[0] = 0.0;
        _data[1] = 0.0;
    }

    void readData() {
        int retry = 0;
        while (retry < 20) {
            ME310::return_t rc = _modem->gps_get_acquired_position();
            if (rc == ME310::RETURN_VALID) {
                char* buff = (char*)_modem->buffer_cstr(1);
                if (buff != nullptr) {
                    float lat = 0.0, lng = 0.0;
                    if (parseLatLng(buff, lat, lng)) {
                        _data[0] = lat;
                        _data[1] = lng;
                        break;
                    }
                }
            }
            delay(5000);
            retry++;
        }
    }

private:
    ME310* _modem;
    unsigned long lastTime;

    public:
    ME310* getModem() { return _modem; } // Added public getter for _modem

    // $GPSACP: 3723.2475,N,12158.3416,W,... 형식에서 위도/경도 추출
    // gps.h 파일 안에 있는 parseLatLng 함수를 이걸로 교체하세요.
    bool parseLatLng(const char* gpsStr, float& lat, float& lng) {
        if (gpsStr == nullptr) return false;

        // ':' 문자 뒤부터 파싱 시작
        const char* p = strchr(gpsStr, ':');
        if (!p) return false;

        float latRaw, lngRaw;
        char latDir, lngDir;

        // [수정된 부분]
        // sscanf 서식을 변경하여 맨 앞의 시간 데이터(%*f)는 읽기만 하고 무시(저장 안함)하도록 함
        // 이렇게 하면 위도, 경도 값을 올바른 변수에 저장할 수 있음
        int n = sscanf(p + 1, " %*f,%f,%c,%f,%c", &latRaw, &latDir, &lngRaw, &lngDir);
        
        // 위도, 남/북, 경도, 동/서 4개 필드가 모두 정상적으로 읽혔는지 확인
        if (n == 4) {
            lat = convertToDecimal(latRaw, latDir);
            lng = convertToDecimal(lngRaw, lngDir);
            return true;
        }
        
        return false;
    }

    // 위도/경도 변환 (ddmm.mmmm -> decimal)
    float convertToDecimal(float raw, char dir) {
        int deg = int(raw / 100);
        float min = raw - deg * 100;
        float dec = deg + min / 60.0;
        if (dir == 'S' || dir == 'W') dec = -dec;
        return dec;
    }
};
