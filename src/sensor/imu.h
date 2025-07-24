#pragma once

#include "../lib/sensor.h"
#include <SoftwareSerial.h>

class IMU : public Sensor<float> {
public:
    // Constructor: sensorMode, dataLength, rxPin, txPin
    IMU(int8_t sensorMode, uint8_t dataLength, byte rxPin, byte txPin)
        : Sensor(sensorMode, dataLength), mpu_ss(rxPin, txPin) {
        // Ensure dataLength is sufficient for Accel (3) + Gyro (3) = 6 floats
        if (dataLength < 3) {
            Serial.println("Warning: IMU dataLength is too small for accelerometer data.");
        }
    }

    void init() {
        mpu_ss.begin(115200);     // Arduino <-> Sensor UART
        mpu_ss.begin(9600);
        delay(100);
        mpu_ss.write('d');  // 센서에게 9600bps 모드로 바꾸라는 명령
        delay(200);
        Serial.println("MPU-6050 Accelerometer Calibration starting...");
        Serial.println("Place the sensor flat and still for 5 seconds...");
        delay(1000); // User preparation time

        calibrateAccelerometer();    // Call calibration function

        Serial.println("Calibration complete!");
        Serial.println("Outputting calibrated acceleration values (X, Y, Z)");
        Serial.println("------------------------------------\n");
    }

    void readData() {
        // Serial.println("IMU readData() entered."); // Debug: Function entry
        // Process incoming data from MPU6050 via SoftwareSerial
        while (mpu_ss.available()) {
            byte incomingByte = mpu_ss.read();
            Serial.print("Received byte: 0x"); // Debug: print every incoming byte
            if (incomingByte < 0x10) Serial.print("0");
            Serial.println(incomingByte, HEX);

            if (bufferIndex == 0 && incomingByte == 0x55) {
                buffer[bufferIndex++] = incomingByte;
                Serial.println("Start byte 0x55 detected."); // Debug
            } else if (bufferIndex > 0 && bufferIndex < 11) {
                buffer[bufferIndex++] = incomingByte;
                if (bufferIndex >= 11) {
                    Serial.print("Full packet received. Checking checksum..."); // Debug
                    if (verifyChecksum(buffer)) {
                        Serial.println("Checksum OK. Parsing data."); // Debug
                        parseData(buffer);
                    } else {
                        Serial.println("Checksum FAILED."); // Debug
                    }
                    bufferIndex = 0; // Reset index for next packet
                }
            } else {
                Serial.print("Invalid byte or sequence. Resetting buffer. Byte: 0x"); // Debug
                if (incomingByte < 0x10) Serial.print("0");
                Serial.println(incomingByte, HEX);
                bufferIndex = 0;
            }
        }
    }

private:
    SoftwareSerial mpu_ss; // SoftwareSerial object
    float acc_offset[3] = {0.0, 0.0, 0.0}; // Calibration offsets
    byte buffer[11]; // Data reception buffer
    byte bufferIndex = 0; // Buffer index

    // Calibration function
    void calibrateAccelerometer() {
        float acc_sum[3] = {0.0, 0.0, 0.0};
        int sample_count = 500; // Number of samples for error measurement

        for (int i = 0; i < sample_count; i++) {
            // Wait until a full 11-byte packet is available
            unsigned long startTime = millis();
            while (mpu_ss.available() < 11) {
                if (millis() - startTime > 1000) { // Timeout after 1 second
                    Serial.println("Calibration timeout: No enough data from MPU6050.");
                    return; // Exit calibration if no data
                }
            }

            byte temp_buffer[11];
            for (int j = 0; j < 11; j++) {
                temp_buffer[j] = mpu_ss.read();
            }

            if (temp_buffer[0] == 0x55 && temp_buffer[1] == 0x51) { // Only process accelerometer data for calibration
                // Checksum verification
                byte sum = 0;
                for (int k = 0; k < 10; k++) sum += temp_buffer[k];
                if (sum == temp_buffer[10]) {
                    acc_sum[0] += (short)(temp_buffer[3] << 8 | temp_buffer[2]);
                    acc_sum[1] += (short)(temp_buffer[5] << 8 | temp_buffer[4]);
                    acc_sum[2] += (short)(temp_buffer[7] << 8 | temp_buffer[6]);
                } else {
                    i--; // Exclude from sample count if checksum fails
                }
            } else {
                i--; // Exclude if not an accelerometer packet
            }
            delay(2); // Small delay between samples
        }

        // Calculate average offset
        acc_offset[0] = acc_sum[0] / sample_count;
        acc_offset[1] = acc_sum[1] / sample_count;
        // Z-axis offset calculation considering gravity (1g = 2048 raw value)
        acc_offset[2] = acc_sum[2] / sample_count - 2048.0;

        Serial.print("Calibration offsets (X, Y, Z): "); // Debug: offsets
        Serial.print(acc_offset[0]); Serial.print(", ");
        Serial.print(acc_offset[1]); Serial.print(", ");
        Serial.println(acc_offset[2]);
    }

    // Checksum verification function
    bool verifyChecksum(byte* data) {
        byte sum = 0;
        for (int i = 0; i < 10; i++) {
            sum += data[i];
        }
        return sum == data[10];
    }

    // Parse the 11-byte packet and populate _data array
    void parseData(byte* data) {
        // Serial.print("Parsing packet type: 0x"); // Debug
        // if (data[1] < 0x10) Serial.print("0");
        // Serial.println(data[1], HEX);

        if (data[1] == 0x51) { // Accelerometer data
            float raw_acc[3];
            raw_acc[0] = (short)(data[3] << 8 | data[2]);
            raw_acc[1] = (short)(data[5] << 8 | data[4]);
            raw_acc[2] = (short)(data[7] << 8 | data[6]);

            Serial.print("Raw Accel: "); // Debug: raw data
            Serial.print(raw_acc[0]); Serial.print(", ");
            Serial.print(raw_acc[1]); Serial.print(", ");
            Serial.println(raw_acc[2]);

            // Apply calibration offset and convert to 'g' units
            if (_dataLength >= 3) { // Ensure _data array is large enough for accel
                _data[0] = (raw_acc[0] - acc_offset[0]) / 32768.0 * 16.0;
                _data[1] = (raw_acc[1] - acc_offset[1]) / 32768.0 * 16.0;
                _data[2] = (raw_acc[2] - acc_offset[2]) / 32768.0 * 16.0;
            } else {
                Serial.println("Error: _data array too small for IMU accelerometer data.");
            }
        }
        // Add more else if blocks for other data types (e.g., 0x53 for angle) if needed.
    }

        
};