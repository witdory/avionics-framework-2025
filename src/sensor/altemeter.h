#pragma once
#include "../lib/sensor.h"
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>

class ALTEMETER : public Sensor<float> {
public:
    ALTEMETER(int8_t sensorMode, int dataLength, int8_t cs_pin) : Sensor(sensorMode, dataLength), bmp(cs_pin) {}

    void init() {
        if (!bmp.begin()) {
            Serial.println("BMP280 Sensor Error: Could not find a valid BMP280 sensor, check wiring!");
            //while (1); // 이 주석을 해제하면 센서 초기화 실패 시 프로그램이 멈춥니다.
        }
        else {
            Serial.println("BMP280 Sensor Initialized Successfully.");
        }
        bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,
                        Adafruit_BMP280::SAMPLING_X2,
                        Adafruit_BMP280::SAMPLING_X16,
                        Adafruit_BMP280::FILTER_X16,
                        Adafruit_BMP280::STANDBY_MS_500);
        initialAltitude = resetInitialAltitude();
        Serial.print("Initial Altitude (resetInitialAltitude): ");
        Serial.println(initialAltitude);
        _data[0] = 0.0;
    }

    float resetInitialAltitude() {
        float mean_init_altitude = 0;
        for(int i=0;i<10;i++){
            float current_altitude = bmp.readAltitude(SEALEVELPRESSURE_HPA);
            mean_init_altitude+=current_altitude;
            Serial.print("Calibration Altitude Sample "); Serial.print(i); Serial.print(": "); Serial.println(current_altitude);
            delay(100);
        }
        mean_init_altitude/=10;
        return mean_init_altitude;
    }

    void readData() {
        float current_raw_altitude = bmp.readAltitude(SEALEVELPRESSURE_HPA);
        float altitude = current_raw_altitude - initialAltitude;
        _data[0] = altitude;
        Serial.print("Raw Altitude: "); Serial.print(current_raw_altitude);
        Serial.print(", Calculated Altitude: "); Serial.println(altitude);
        delay(10);
    }

private:
    Adafruit_BMP280 bmp;
    float initialAltitude;
    const float SEALEVELPRESSURE_HPA = 1006.0; //해수면 기준 고흥 기압 -> 장소가 달라지면 해당 지역 해면 기압 입력  
};
