#pragma once
#include "../modules.h"
#include <Arduino.h>
#include "../lte/lte_http.h"

#define SERVER "114.108.80.58"
#define PORT 10025
#define APN "simplio.apn"

class Transmit : public Task{
public:
    Transmit(IMU *imu, ALTEMETER *alt, GPS *gps, LOGGER *logger, me310::ME310* modem)
    {
        _imu = imu;
        _alt = alt;
        _gps = gps;
        _logger = logger;
        _modem = modem;
        _D_Len = (_imu->getDataLength() + _alt->getDataLength() + _gps->getDataLength())*4;
        _data = new uint8_t[_D_Len];
        _cnt = 0;
        log = "";
    }

    /*
        데이터 구조
        [gyrox, gyroy, gyroz, accx, accy, accz, alt, lat, lon]
        모두 float형태로 4byte씩 할당
    */

    // 센서 데이터 전송 (LTE HTTP POST)
    void sendSensorData(Stage currentStage){
        _updateData();
        String data;
        if (currentStage == RETRIEVAL) {
            data = String((*_imu)._data[0])+","+String((*_imu)._data[1])+","+String((*_imu)._data[2])+","+String((*_imu)._data[3])+","+String((*_imu)._data[4])+","+String((*_imu)._data[5])+","+String((*_alt)._data[0])+","+String((*_gps)._data[0],8)+","+String((*_gps)._data[1],8);
        } else {
            data = String((*_imu)._data[0])+","+String((*_imu)._data[1])+","+String((*_imu)._data[2])+","+String((*_imu)._data[3])+","+String((*_imu)._data[4])+","+String((*_imu)._data[5])+","+String((*_alt)._data[0])+","+"0.0,0.0"; // Placeholder for GPS
        }
        Serial.println(data);
        lte_http_post(*_modem, APN, SERVER, PORT, "/data", data.c_str());
        unsigned long currentTime = millis();
        String sensorlog = "1,"+String(currentTime)+","+data+"\n";
        log+=sensorlog;
        _cnt+=1;
        if(_cnt>=30){
            _cnt = 0;
            _logger->writeData(log);
            log = "";
        }
    }

    // 로켓 상태 전송 (LTE HTTP POST)
    void sendRocketState(Stage stage) {
        String stageStr;
        switch(stage) {
            case INIT: stageStr = "INIT"; break;
            case READY: stageStr = "READY"; break;
            case ASCENDING: stageStr = "ASCENDING"; break;
            case APOGEE: stageStr = "APOGEE"; break;
            case DESCENDING: stageStr = "DESCENDING"; break;
            default: stageStr = "UNKNOWN"; break;
        }
        lte_http_post(*_modem, APN, SERVER, PORT, "/state", stageStr.c_str());
        Serial.print("Rocket state sent: ");
        Serial.println(stageStr);
    }

private:
    me310::ME310* _modem;
    IMU *_imu;
    ALTEMETER *_alt;
    GPS *_gps;
    LOGGER *_logger;



    uint8_t *_data;
    uint8_t _D_Len;
    uint8_t _cnt;
    String log;
    void _updateData(){
        int idx = 0;
        for(int i=0;i<_imu->getDataLength();i++){
            uint8_t temp[4];
            memcpy(temp, &((*_imu)._data[i]), sizeof(float));
            for(int j=0;j<4;j++){
                _data[idx] = temp[j];
                idx++;
            }
        }
        for(int i=0;i<_alt->getDataLength();i++){
            uint8_t temp[4];
            memcpy(temp, &(*_alt)._data[i], sizeof(float));
            for(int j=0;j<4;j++){
                _data[idx] = temp[j];
                idx++;
            }
        }
        for(int i=0;i<_gps->getDataLength();i++){
            uint8_t temp[4];
            memcpy(temp, &(*_gps)._data[i], sizeof(float));
            for(int j=0;j<4;j++){
                _data[idx] = temp[j];
                idx++;
            }
        }
    }
};