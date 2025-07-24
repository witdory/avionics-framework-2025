#pragma once
#include "../modules.h"

class UpdateSensor : public Task{
public:
    UpdateSensor(IMU *imu, ALTEMETER *alt, GPS *gps){
        _imu = imu;
        _alt = alt;
        _gps = gps;
    }

    inline void run(Stage currentStage){
        _imu->readData();
        delay(30);
        if (currentStage == RETRIEVAL) {
            _gps->readData();
        }
        delay(30);
        _alt->readData();
        delay(20);
    }

    void calibration(){
        //센서들 캘리브레이션 코드 작성할것
    }


private:
    IMU *_imu;
    ALTEMETER *_alt;
    GPS *_gps;
};