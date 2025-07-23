#pragma once
#include<SPI.h>
#include<SD.h>
#include"../lib/sensor.h"

class LOGGER : public Sensor<bool>{
public:
    LOGGER(int chipSelect) : Sensor<bool>(READ_AND_WRITE) {
        _chipSelect = chipSelect;
        filename = "flightLogger.txt";
    }

    void init(){
        if (!SD.begin(_chipSelect)) {
            Serial.println("SD card initialization failed!");
            return; // Exit if SD card fails to initialize
        }

        // Ensure flightLogger.txt is cleared at the start of a new flight
        if (SD.exists("flightLogger.txt")) {
            SD.remove("flightLogger.txt");
        }

        int flightCount = 1;
        if (SD.exists("logcnt.txt")) {
            File myFile = SD.open("logcnt.txt", FILE_READ);
            if (myFile) {
                String numberString = myFile.readStringUntil('\n');
                myFile.close();
                flightCount = numberString.toInt();
                if (flightCount == 0) flightCount = 1; // Handle case where file is empty or contains 0
            } else {
                Serial.println("Error opening logcnt.txt for reading.");
            }
        }
        
        // Increment flight count for the new flight
        flightCount++;

        // Write updated flight count back to logcnt.txt
        File myFile = SD.open("logcnt.txt", FILE_WRITE);
        if (myFile) {
            myFile.println(flightCount);
            myFile.close();
        } else {
            Serial.println("Error opening logcnt.txt for writing.");
        }
        return;
        return;
    }

    void writeData(String data){
        File dataFile = SD.open("log.txt", FILE_WRITE);
        if(dataFile){
            dataFile.println(data);
            dataFile.close();
        }
        else{
            Serial.println("fail");
            //delay(10000);
            return;
        }
    }

    void readData(){
        File dataFile = SD.open("flightLogger.txt", FILE_READ);
        if (dataFile) {
            Serial.println("Reading from file:");
            while (dataFile.available()) {
                Serial.write(dataFile.read());
            }
            dataFile.close();
        } else {
            Serial.println("Error opening file for reading.");
        }
    }

private:
    int _chipSelect;
    String filename;
};