#include "../lib/sensor.h"
#include "../lib/task.h"
#include "../lib/stage.h"
#include "../sensor/imu.h"
#include "../sensor/altemeter.h"
#include "../sensor/gps.h"
#include "../sensor/logger.h"
#include <Arduino.h>
#include "../lte/HttpClient.h"

#define SERVER "114.108.80.58"
#define PORT 10025
#define APN "simplio.apn"

class Transmit : public Task{
public:
    Transmit(IMU *imu, ALTEMETER *alt, GPS *gps, LOGGER *logger, PersistentTCP* tcp)
    {
        _imu = imu;
        _alt = alt;
        _gps = gps;
        _logger = logger;
        _tcp = tcp;
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
        
        unsigned long currentTime = millis();
        String sensorlog = currentStage+","+String(currentTime)+","+data+"\n";
        log+=sensorlog;

        _cnt+=1;
        if(_cnt>=30){
            _cnt = 0;
            Serial.println("Transmit: Writing log to SD card.");
            // _logger->writeData(log);
            // ASCENDING 단계에서는 서버로 전송하지 않음
            // if (currentStage >= APOGEE){
            //     _tcp->send(("SENSOR_DATA:" + log).c_str()); // SENSOR_DATA 접두사 추가
            //     _tcp->receive(); // 응답 소비
            // }
            log = ""; // SD 카드에 쓴 후 버퍼 비우기
        }
    }

    void sendSdCardDataToServer() {
        Serial.println("Transmit: Starting SD card data transfer to server.");
        _tcp->send("ASCENDING_DATA: ");
        if (_logger->openLogFileForReading()) {
            String dataChunk;
            const int linesPerChunk = 20; // 10~30줄 사이, 20줄로 설정
            int chunkCount = 0;
            while (true) {
                dataChunk = _logger->readDataChunk(linesPerChunk);
                if (dataChunk.length() == 0) {
                    break; // End of file
                }
                Serial.print("Transmit: Sending chunk ");
                Serial.print(++chunkCount);
                Serial.print(" (length: ");
                Serial.print(dataChunk.length());
                Serial.println(") to server.");
                _tcp->send(("SD_DATA_CHUNK:" + dataChunk).c_str()); // SD_DATA_CHUNK 접두사 추가
                _tcp->receive(); // 응답 소비
                delay(100); // 서버 부하를 줄이기 위한 딜레이
            }
            _logger->closeLogFileForReading();
            Serial.println("Transmit: Finished SD card data transfer.");
        } else {
            Serial.println("Transmit: Failed to open log.txt for reading.");
        }
    }

private:
    PersistentTCP* _tcp;
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