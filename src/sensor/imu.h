#pragma once

#include "../lib/sensor.h"
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

class IMU : public Sensor<float> {
public:
    // Constructor: sensorMode, dataLength (I2C does not use rxPin, txPin)
    IMU(int8_t sensorMode, uint8_t dataLength)
        : Sensor(sensorMode, dataLength), mpu(), _alpha(0.98), _dt(0.01) { // Initialize mpu object, set filter alpha and dt
        // Ensure dataLength is sufficient for Pitch (1) + Roll (1) + AccelZ (1) = 3 floats
        if (dataLength < 3) {
            Serial.println("Warning: IMU dataLength is too small for Pitch, Roll, and AccelZ data.");
        }
        _angleX = 0.0; // Pitch
        _angleY = 0.0; // Roll
        _lastTime = millis();
    }

    void init() {
        Wire.begin(); // Initialize I2C communication

        Serial.println("\n--- IMU Initialization ---");

        // Try to initialize MPU6050
        if (!mpu.begin()) {
            Serial.println("ERROR: Unable to communicate with MPU-6050");
            Serial.println("       Check wiring and I2C address!");
            //while (1); // Uncomment to halt on error
        }
        Serial.println("MPU-6050 Found and Connected!");

        mpu.setAccelerometerRange(MPU6050_RANGE_8_G); // Set accelerometer range to 8G
        mpu.setGyroRange(MPU6050_RANGE_500_DEG);     // Set gyroscope range to 500 deg/s
        mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);  // Set filter bandwidth to 21Hz

        Serial.println("MPU-6050 Initialized Successfully.");
        Serial.println("--------------------------");
        delay(100);
    }

    void readData() {
        sensors_event_t a, g, temp;
        mpu.getEvent(&a, &g, &temp);

        // Calculate time difference
        unsigned long currentTime = millis();
        _dt = (currentTime - _lastTime) / 1000.0; // Convert to seconds
        _lastTime = currentTime;

        // Accelerometer angles (degrees)
        float accelAngleX = atan2(a.acceleration.y, sqrt(a.acceleration.x * a.acceleration.x + a.acceleration.z * a.acceleration.z)) * 180 / PI;
        float accelAngleY = atan2(a.acceleration.x, sqrt(a.acceleration.y * a.acceleration.y + a.acceleration.z * a.acceleration.z)) * 180 / PI;

        // Gyroscope angles (degrees/second)
        float gyroX = g.gyro.x;
        float gyroY = g.gyro.y;
        float gyroZ = g.gyro.z;

        // Complementary filter
        _angleX = _alpha * (accelAngleX + gyroX * _dt) + (1 - _alpha) * accelAngleX; // Pitch
        _angleY = _alpha * (accelAngleY + gyroY * _dt) + (1 - _alpha) * accelAngleY; // Roll

        // Populate _data array
        if (_dataLength >= 3) {
            _data[0] = _angleX; // Pitch
            _data[1] = _angleY; // Roll
            _data[2] = a.acceleration.z; // Z-axis acceleration (m/s^2)

            // Serial.print("IMU Accel (m/s^2): X="); Serial.print(a.acceleration.x, 2);
            // Serial.print(", Y="); Serial.print(a.acceleration.y, 2);
            // Serial.print(", Z="); Serial.println(a.acceleration.z, 2);

            // Serial.print("Raw Accel: X="); Serial.print(a.acceleration.x);
            // Serial.print(", Y="); Serial.print(a.acceleration.y);
            // Serial.print(", Z="); Serial.println(a.acceleration.z);

            // Serial.print("Raw Gyro: X="); Serial.print(g.gyro.x);
            // Serial.print(", Y="); Serial.print(g.gyro.y);
            // Serial.print(", Z="); Serial.println(g.gyro.z);

            Serial.print("IMU Euler (deg): Pitch="); Serial.print(_data[0], 2);
            Serial.print(", Roll="); Serial.print(_data[1], 2);
            Serial.print(", Yaw=N/A"); // MPU6050 does not have magnetometer for Yaw
            Serial.println(", AccelZ="); Serial.println(_data[2], 2);

        } else {
            Serial.println("Error: _data array too small for IMU data.");
        }
    }

private:
    Adafruit_MPU6050 mpu; // MPU6050 object
    float _angleX, _angleY; // Filtered angles (Pitch, Roll)
    float _alpha; // Complementary filter coefficient
    float _dt; // Time step for integration
    unsigned long _lastTime;
};