#pragma once
#include<SPI.h>
#include<SdFat.h>
#include"../lib/sensor.h"

class LOGGER : public Sensor<bool>{
public:
    LOGGER(int8_t cs_pin) : Sensor<bool>(READ_AND_WRITE), _chipSelect(cs_pin) {
        filename = "flightLogger.txt";
    }

    void init(){
        Serial.print("Attempting SD card initialization with CS pin: ");
        Serial.println(_chipSelect);
        if (!sd.begin(_chipSelect, SD_SCK_MHZ(4))) {
            Serial.println("SD card initialization failed!");
            return; // Exit if SD card fails to initialize
        }

        // Ensure log.txt is cleared at the start of a new flight
        if (sd.exists("log.txt")) {
            sd.remove("log.txt");
        }

        int flightCount = 1;
        if (sd.exists("logcnt.txt")) {
            FsFile myFile = sd.open("logcnt.txt", O_READ);
            if (myFile) {
                String numberString = "";
                char c;
                while (myFile.read(&c, 1) == 1 && c != '\n') {
                    numberString += c;
                }
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
        FsFile myFile = sd.open("logcnt.txt", O_WRITE | O_CREAT | O_TRUNC);
        if (myFile) {
            myFile.println(flightCount);
            myFile.close();
        } else {
            Serial.println("Error opening logcnt.txt for writing.");
        }
        return;
    }

    void writeData(String data){
        FsFile dataFile = sd.open("log.txt", O_RDWR | O_CREAT | O_AT_END);
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

    bool openLogFileForReading() {
        _logFile = sd.open("log.txt", O_READ);
        if (!_logFile) {
            Serial.println("Error opening log.txt for reading.");
            return false;
        }
        return true;
    }

    String readDataChunk(int numLines) {
        String chunk = "";
        int linesRead = 0;
        while (_logFile.available() && linesRead < numLines) {
            char c;
            while (_logFile.read(&c, 1) == 1 && c != '\n') {
                chunk += c;
            }
            if (_logFile.peek() != -1) { // Add newline if not the end of file
                chunk += '\n';
            }
            linesRead++;
        }
        return chunk;
    }

    void closeLogFileForReading() {
        if (_logFile) {
            _logFile.close();
        }
    }

private:
    int _chipSelect;
    String filename;
    SdFat sd; // Added SdFat object
    FsFile _logFile; // Changed File to FsFile
};