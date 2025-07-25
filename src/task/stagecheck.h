#pragma once
#include "../modules.h"
#include <Arduino.h>

class StageChecker : public Task{
public:
    StageChecker(IMU *imu, ALTEMETER *alt, GPS *gps, Stage *stage, LOGGER *logger) :
        _imu(imu),
        _alt(alt),
        _gps(gps),
        _stage(stage),
        _logger(logger),
        ascendingTime(0),
        _apogeeToDescendingTransitioned(false),
        descendingStartTime(0) // Initialize descendingStartTime to 0
    {}

    void run(){
        if(*_stage == READY){
            // Z축 가속도가 5.0 m/s^2 이상일 때 ASCENDING으로 전환
            if(_imu->_data[2] >= 15.0){
                Serial.println("Stage,ASCENDING");
                *_stage = ASCENDING;
                ascendingTime = millis();
                String log = "ASCENDING";                // _logger->writeData(log);
                return;
            }
        }
        else if(*_stage == ASCENDING){
            // 낙하산 사출 조건 확인

            // 1. pitch 혹은 roll 값이 75도 이상 기울었을 때
            if(abs(int(_imu->_data[0])) >= 75 || abs(int(_imu->_data[1])) >= 75){
                Serial.println("===============[[[Stage,APOGEE]]]==============");
                Serial.println(abs(int(_imu->_data[0]))); // Pitch 값 출력
                *_stage = APOGEE;
                String log = "APOGEE1\n";
                // _logger->writeData(log);
            }

            // 2. 로켓의 하강이 감지 될 때 (Z축 가속도가 -2.0 m/s^2 이하일 때)
            else if(_imu->_data[2] <= -2.0){
                Serial.println("Stage,APOGEE");
                *_stage = APOGEE;
                String log = "APOGEE2\n";
                // _logger->writeData(log);
            }

            // 3. 타이머의 값이 발사 후 8초가 지났을때 (발사 시뮬레이터 기반으로 값 조정 예정)
            else if((millis()-ascendingTime)/1000>8){
                Serial.println("Stage,APOGEE");
                *_stage = APOGEE;
                String log = "APOGEE3\n";
                // _logger->writeData(log);
            }

            return;
        }
        else if(*_stage == APOGEE){
            if (!_apogeeToDescendingTransitioned) {
                Serial.println("Stage,DESCENDING");
                *_stage = DESCENDING;
                String log = "DESCENDING\n";                // _logger->writeData(log);
                _apogeeToDescendingTransitioned = true; // Set flag to prevent repeated transition
                descendingStartTime = millis(); // Initialize descending start time
                Serial.print("APOGEE -> DESCENDING: descendingStartTime set to ");
                Serial.println(descendingStartTime);
            }
            return;
        }
        else if(*_stage == DESCENDING){
            // Define thresholds for retrieval
            const unsigned long TIME_TO_RETRIEVAL_THRESHOLD = 10000; // 10 seconds in milliseconds
            const float ALTITUDE_TOLERANCE = 5.0; // meters

            // Condition 1: Sufficient time has passed since descending started
            bool timeElapsed = (millis() - descendingStartTime >= TIME_TO_RETRIEVAL_THRESHOLD);

            // Condition 2: Current altitude is close to initial launch altitude
            bool altitudeCloseToInitial = (abs(_alt->_data[0] - _alt->getInitialAltitude()) < ALTITUDE_TOLERANCE);

            // if (timeElapsed || altitudeCloseToInitial) {
            Serial.print("DESCENDING: millis()="); Serial.print(millis());
            Serial.print(", descendingStartTime="); Serial.print(descendingStartTime);
            Serial.print(", Elapsed="); Serial.print(millis() - descendingStartTime);
            Serial.print(", Threshold="); Serial.print(TIME_TO_RETRIEVAL_THRESHOLD);
            Serial.print(", timeElapsed="); Serial.println(timeElapsed ? "true" : "false");

            if (timeElapsed ) {
                Serial.println("Stage,RETRIEVAL");
                *_stage = RETRIEVAL;
                String log = "RETRIEVAL\n";
                // _logger->writeData(log);
            }
        }
    }

    void setAscendingTime(unsigned long Ascendingtime){
        ascendingTime = Ascendingtime;
    }

    void setDescendingStartTime(unsigned long DescendingStartTime){
        descendingStartTime = DescendingStartTime;
    }

private:
    IMU *_imu;
    ALTEMETER *_alt;
    GPS *_gps;
    Stage *_stage;
    LOGGER *_logger;

    unsigned long ascendingTime;
    bool _apogeeToDescendingTransitioned = false; // New flag to control transition
    unsigned long descendingStartTime; // Added for DESCENDING stage
};