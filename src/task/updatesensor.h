#pragma once
#include "../modules.h"

class UpdateSensor : public Task{
public:
    UpdateSensor(IMU *imu, ALTEMETER *alt, GPS *gps){
        _imu = imu;
        _alt = alt;
        _gps = gps;
    }

    inline void run(){
        //Serial.println("센서 업데이트 시작");
        _imu->readData();
        delay(30);
        _gps->readData();
        delay(30);
        _alt->readData();
        delay(20);
    }

private:
    IMU *_imu;
    ALTEMETER *_alt;
    GPS *_gps;
};