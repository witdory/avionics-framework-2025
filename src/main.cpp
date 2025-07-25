#include "modules.h"
#include "task/transmit.h"
#include <SoftwareSerial.h>

// Pin definitions
#define MPU_RX 5
#define MPU_TX 6
#define BMP_CS 10
#define SD_CS 8
#define PARACHUTE_PIN 9

Modem modem;
HttpClient httpClient(&modem);
PersistentTCP tcp(&modem, "114.108.80.58", 10025);



ALTEMETER alt(READ_ONLY, 1, BMP_CS);
GPS gps(READ_ONLY, 2, modem.getModem());
IMU imu(READ_ONLY, 3);
MOTOR parachute_motor(WRITE_ONLY, 180, PARACHUTE_PIN);
LOGGER logger(SD_CS);
Stage stage;

bool sdDataSentApogee = false;

Parachute parachute(&parachute_motor);
Transmit transmit(&imu, &alt, &gps, &logger, &tcp);
UpdateSensor updatesensor(&imu, &alt, &gps);
StageChecker stagecheck(&imu, &alt, &gps, &stage,&logger);

// 이전 코드...



unsigned long initTime, currentTime; 

inline void getCommand(String command){
    // Serial.print("getCommand called with: ");
    // Serial.println(command);
    if (command == "Ready") {
        stage = READY;
        Serial.println("Stage changed to: READY (from Ready command)");
        String stageStr = "READY";
        // httpClient.post(SERVER, 8000, "/state", stageStr.c_str());
    } else if (command == "Injection") {
        //parachute.run();
        stage = APOGEE;
        Serial.println("Stage changed to: APOGEE (from Injection command)");
        String stageStr = "APOGEE";
        // httpClient.post(SERVER, 8000, "/state", stageStr.c_str());
    } else if (command == "StageChange") {
        switch (stage)
        {
        case INIT: {
            stage = CALIBRATION;
            Serial.println("Stage changed to: CALIBRATION (from StageChange command, was INIT)");
            // String stageStrInit = "READY";
            // httpClient.post(SERVER, 8000, "/state", stageStrInit.c_str());
            break;
        }
        case CALIBRATION: {
            stage = READY;
            Serial.println("Stage changed to: READY (from StageChange command, was CALIBRATION)");
            // String stageStrInit = "READY";
            // httpClient.post(SERVER, 8000, "/state", stageStrInit.c_str());
            break;
        }
        case READY: {
            stage = ASCENDING;
            Serial.println("Stage changed to: ASCENDING (from StageChange command, was READY)");
            stagecheck.setAscendingTime(millis());
            // String stageStrReady = "ASCENDING";
            // httpClient.post(SERVER, 8000, "/state", stageStrReady.c_str());
            break;
        }
        case ASCENDING: {
            stage = APOGEE;
            Serial.println("Stage changed to: APOGEE (from StageChange command, was ASCENDING)");
            String stageStrAscending = "APOGEE";
            // httpClient.post(SERVER, 8000, "/state", stageStrAscending.c_str());
            break;
        }
        case APOGEE: {
            stage = DESCENDING;
            Serial.println("Stage changed to: DESCENDING (from StageChange command, was APOGEE)");
            String stageStrApogee = "DESCENDING";
            // httpClient.post(SERVER, 8000, "/state", stageStrApogee.c_str());
            break;
        }
        default: {
            break;
        }}
    }
}

void setup(){
    //초기 시간 저장
    initTime = millis();
    // 초기 stage INIT으로 초기화
    stage = INIT;
    // Serial.println("Stage changed to: INIT");
    pinMode(10, OUTPUT); // BMP280 CS
    pinMode(8, OUTPUT);  // SD카드 CS
    digitalWrite(10, HIGH); // BMP280 비활성화
    digitalWrite(8, HIGH);  // SD카드 비활성화

    // 센서들 연결 및 초기화
    Serial.begin(115200);
    
    Serial.println("INIT");
    gps.init(); // GPS init is always called
    Serial.println("GPS DONE");
    imu.init();
    Serial.println("IMU DONE");
    digitalWrite(10, HIGH); // BMP280 CS 비활성화
    // logger.init();
    Serial.println("SD DONE");
    digitalWrite(8, HIGH); // SD카드 CS 비활성화
    alt.init();
    Serial.println("ALT DONE");
    
    // parachute_motor.init2();
    Serial.println("MOTOR");

    if(modem.init(APN)){
        Serial.println("LTE DONE");
    } else {
        Serial.println("LTE FAILED");
    }

    Serial.println("Attempting TCP connect...");
    if (tcp.connect()) {
        Serial.println("TCP 연결 성공!");
        // TCP 연결 성공 후 초기 상태를 서버로 HTTP POST 전송 (포트 8000)
        String stageStr = "INIT";
        // httpClient.post(SERVER, 8000, "/state", stageStr.c_str());
        // TCP 연결 성공 후 초기 상태를 PersistentTCP (포트 10025)로 전송
        Serial.println("Sending INIT_STATE:INIT...");
        if (tcp.send("INIT_STATE:INIT")) {
            Serial.println("INIT_STATE:INIT sent successfully.");
        } else {
            Serial.println("INIT_STATE:INIT send failed.");
        }
    } else {
        Serial.println("TCP 연결 실패");
    }

    // 지상국으로 초기화 완료 되었다고 데이터 전송
    Serial.println("INIT");
    // delay(100);
    Serial.println("Stage, INIT");
    String log = "INIT\n";
    // logger.writeData(log);
    digitalWrite(13, LOW);
}

void loop(){
    Serial.print("Stage: ");
    Serial.println(stage);
    
    String command; // Declare command here for broader scope

    if(stage == INIT){
        updatesensor.run(stage);

        // 1. TCP 연결 확인 및 재연결
        if (!tcp.isConnected()) {
            Serial.println("TCP not connected. Attempting to reconnect...");
            if (!tcp.connect()) {
                Serial.println("TCP reconnection failed. Skipping loop.");
                delay(5000);
                return;
            }
            Serial.println("TCP reconnected successfully.");
        }

        // 2. 서버에 명령 요청
        tcp.send("COMMAND_REQUEST:STAGE:INIT");
        
        // 3. 서버로부터 응답 수신
        String raw_response = "";
        unsigned long receiveStartTime = millis();
        while (millis() - receiveStartTime < 5000) { // 5초 타임아웃
            raw_response = tcp.receive();
            if (raw_response.length() > 0) {
                break;
            }
            delay(100);
        }
        
        // 4. 수신된 데이터에서 실제 명령어 파싱
        String parsed_command = "";
        if (raw_response.indexOf("StageChange") != -1) {
            parsed_command = "StageChange";
        } else if (raw_response.indexOf("Ready") != -1) {
            parsed_command = "Ready";
        } else if (raw_response.indexOf("Injection") != -1) {
            parsed_command = "Injection";
        } else if (raw_response.indexOf("OK") != -1) {
            parsed_command = "OK";
        }
        
        Serial.print("Parsed command: '"); Serial.print(parsed_command); Serial.println("'");

        // 5. 파싱된 명령어로 로직 처리
        if (parsed_command.length() > 0) {
            if (parsed_command != "OK") {
                getCommand(parsed_command);
            } else {
                Serial.println("Received OK ack, holding stage.");
            }
        } else {
            Serial.println("No valid command keyword found in response.");
        }
    }
    else if(stage == CALIBRATION){
        updatesensor.run(stage);

        // 1. TCP 연결 확인 및 재연결
        if (!tcp.isConnected()) {
            Serial.println("TCP not connected. Attempting to reconnect...");
            if (!tcp.connect()) {
                Serial.println("TCP reconnection failed. Skipping loop.");
                delay(5000);
                return;
            }
            Serial.println("TCP reconnected successfully.");
        }

        // 2. 서버에 명령 요청
        tcp.send("COMMAND_REQUEST:STAGE:CALIBRATION");
        
        // 3. 서버로부터 응답 수신
        String raw_response = "";
        unsigned long receiveStartTime = millis();
        while (millis() - receiveStartTime < 5000) { // 5초 타임아웃
            raw_response = tcp.receive();
            if (raw_response.length() > 0) {
                break;
            }
            delay(100);
        }
        
        // 4. 수신된 데이터에서 실제 명령어 파싱
        String parsed_command = "";
        if (raw_response.indexOf("StageChange") != -1) {
            parsed_command = "StageChange";
        } else if (raw_response.indexOf("Ready") != -1) {
            parsed_command = "Ready";
        } else if (raw_response.indexOf("Injection") != -1) {
            parsed_command = "Injection";
        } else if (raw_response.indexOf("OK") != -1) {
            parsed_command = "OK";
        }
        
        Serial.print("Parsed command: '"); Serial.print(parsed_command); Serial.println("'");

        // 5. 파싱된 명령어로 로직 처리
        if (parsed_command.length() > 0) {
            if (parsed_command != "OK") {
                getCommand(parsed_command);
            } else {
                Serial.println("Received OK ack, holding stage.");
            }
        } else {
            Serial.println("No valid command keyword found in response.");
        }
    }
    else if(stage == READY){
        // Serial.println("Entering READY stage block.");
        // 텔레메트리 값 읽어오기 시작
        updatesensor.run(stage);

        // Serial.print("Altemeter: ");
        // Serial.println(alt._data[0]);

        // 읽은 텔레메트리 값 전송 및 저장
        // transmit.sendSensorData(stage); // Call directly

        // 만약 가속도 값이 크게 변화하면 상태 ASCENDING으로 바꿈 및 상태 변경 패킷 전송 및 저장
        stagecheck.run();
        // stage = ASCENDING;
        if (stage == ASCENDING){
            Serial.println("ASCENDING TRANSMIT");
            Serial.print("TCP Connected: "); Serial.println(tcp.isConnected() ? "true" : "false");
            if (!tcp.isConnected()) {
                Serial.println("TCP not connected. Attempting to reconnect...");
                if (!tcp.connect()) {
                    Serial.println("TCP reconnection failed. Skipping send/receive.");
                    delay(5000); // Wait before next attempt
                    return; // Skip current loop iteration
                }
                Serial.println("TCP reconnected successfully.");
            }
            if (tcp.send("COMMAND_REQUEST:STAGE:ASCENDING")) {
                Serial.println("ASCENDING TRANSMITTED successfully.");
            } else {
                Serial.println("ASCENDING TRANSMITTED failed.");
            }
        }
    }
    else if(stage == ASCENDING){
        Serial.println("Entering ASCENDING stage block.");
        // 텔레메트리 값 읽기 시작
        updatesensor.run(stage);
        // 텔레메트리 값 전송 및 저장
        
        // 읽은 텔레메트리 값 전송 및 저장
        Serial.print("TCP Connected: "); Serial.println(tcp.isConnected() ? "true" : "false");
        if (!tcp.isConnected()) {
            Serial.println("TCP not connected. Attempting to reconnect...");
            if (!tcp.connect()) {
                Serial.println("TCP reconnection failed. Skipping send/receive.");
                delay(5000); // Wait before next attempt
                return; // Skip current loop iteration
            }
            Serial.println("TCP reconnected successfully.");
        }
        transmit.sendSensorData(stage); // Call directly
        Serial.println("ASCENDING");
        stagecheck.run();
        
    }
    else if(stage == APOGEE){
        Serial.println("Entering APOGEE stage block.");
        // 텔레메트리 값 읽기 시작
        updatesensor.run(stage);
        // 낙하산 사출
        parachute.run();
        // delay(100);
        // APOGEE 진입 시 SD 카드 데이터 전송 (한 번만 실행)
        if (!sdDataSentApogee) {
            Serial.println("상승 데이터 전송");
            // transmit.sendSdCardDataToServer();
            sdDataSentApogee = true;
            stage = DESCENDING;
            stagecheck.setDescendingStartTime(millis()); // Initialize descendingStartTime here
        }
        transmit.sendSensorData(stage); // Call directly

        // stagecheck.run();

        
    }
    else if(stage == DESCENDING){
        // Serial.println("Entering DESCENDING stage block.");
        // 텔레메트리 값 읽기 시작
        updatesensor.run(stage);
        // 텔레메트리 값 전송 및 저장
        Serial.print("TCP Connected: "); Serial.println(tcp.isConnected() ? "true" : "false");
        if (!tcp.isConnected()) {
            Serial.println("TCP not connected. Attempting to reconnect...");
            if (!tcp.connect()) {
                Serial.println("TCP reconnection failed. Skipping send/receive.");
                delay(5000); // Wait before next attempt
                return; // Skip current loop iteration
            }
            Serial.println("TCP reconnected successfully.");
        }
        transmit.sendSensorData(stage); // Call directly
        stagecheck.run();
    }
    else if(stage == RETRIEVAL){
        updatesensor.run(stage);
        
        modem.getModem()->gnss_controller_power_management(0);
        transmit.sendSensorData(stage); // Call directly
        modem.getModem()->gnss_controller_power_management(1);

        // GPS 데이터 획득 및 전송
        Serial.println("RETRIEVAL: Attempting to get GPS data...");
        me310::ME310::return_t rc_gps = gps.getModem()->gps_get_acquired_position();
        if (rc_gps == me310::ME310::RETURN_VALID) {
            String gpsData = (String)gps.getModem()->buffer_cstr_raw();
            Serial.print("RETRIEVAL: GPS Data: "); Serial.println(gpsData);
            if (tcp.isConnected()) {
                if (tcp.send(("GPS_DATA:" + gpsData).c_str())) {
                    Serial.println("RETRIEVAL: GPS data sent successfully.");
                } else {
                    Serial.println("RETRIEVAL: GPS data send failed.");
                }
            } else {
                Serial.println("RETRIEVAL: TCP not connected, cannot send GPS data.");
            }
        } else {
            Serial.print("RETRIEVAL: Failed to get GPS data. RC: "); Serial.println(rc_gps);
        }
    
    }
    delay(10);
}