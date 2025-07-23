#include "modules.h"
#include "task/transmit.h"

Modem modem;
HttpClient httpClient(&modem);
PersistentTCP tcp(&modem, "114.108.80.58", 10025);

ALTEMETER alt(READ_ONLY, 1);
GPS gps(READ_ONLY, 2, modem.getModem());
IMU bno(READ_ONLY, 6);
MOTOR parachute_motor(WRITE_ONLY, 180, 3);
LOGGER logger(53);
Stage stage;

Parachute parachute(&parachute_motor);
Transmit transmit(&bno, &alt, &gps, &logger, &tcp);
UpdateSensor updatesensor(&bno, &alt, &gps);
StageChecker stagecheck(&bno, &alt, &gps, &stage,&logger);

// \uc774\uc9c0\uc0c1\uad6d \ucf54\ub4dc

//Recieve recieve(&xbee);

unsigned long initTime, currentTime; 

void setup(){
    initTime = millis();
    // 초기 stage INIT으로 초기화
    stage = INIT;
    Serial.println("Stage changed to: INIT");
    pinMode(13, HIGH);
    digitalWrite(13, HIGH);

    // 센서들 연결 및 초기화
    Serial.begin(115200);
    Serial.println("INIT");
    gps.init(); // GPS init is always called
    Serial.println("GPS DONE");
    // bno.init();
    Serial.println("IMU DONE");
    // alt.init();
    Serial.println("ALT DONE");
    // logger.init();
    Serial.println("SD DONE");
    // parachute_motor.init2();
    Serial.println("MOTOR");

    if(modem.init(APN)){
        Serial.println("LTE DONE");
    } else {
        Serial.println("LTE FAILED");
    }
    if (tcp.connect()) {
        Serial.println("TCP 연결 성공!");
        // TCP 연결 성공 후 초기 상태를 서버로 HTTP POST 전송 (포트 8000)
        String stageStr = "INIT";
        // httpClient.post(SERVER, 8000, "/state", stageStr.c_str());
        // TCP 연결 성공 후 초기 상태를 PersistentTCP (포트 10025)로 전송
        tcp.send("INIT_STATE:INIT");
    } else {
        Serial.println("TCP 연결 실패");
    }

    // 지상국으로 초기화 완료 되었다고 데이터 전송
    Serial.println("INIT");
    delay(100);
    Serial.println("Stage,INIT");
    String log = "INIT\n";
    // logger.writeData(log);
    digitalWrite(13, LOW);
    
}

void loop(){
    // Serial.print("Stage: ");
    // Serial.println(stage);

    tcp.send("COMMAND_REQUEST:GET_COMMAND"); // 서버에 명령 요청 메시지 전송
    String command = tcp.receive();
    Serial.print("main.cpp: tcp.receive() returned: <");
    Serial.print(command);
    Serial.println(">");

    Serial.print("main.cpp: TCP is connected: ");
    Serial.println(tcp.isConnected() ? "true" : "false");
    command.trim();
    // Serial.println(command);
    if (command.length() > 0) {
        // Serial.println("[In if]");
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
                stage = READY;
                Serial.println("Stage changed to: READY (from StageChange command, was INIT)");
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
        // 기타 명령 처리
    }
    // Serial.println("main.cpp: After command processing block.");

    if(stage == INIT){
        // Serial.println("INIT Stage: Before updatesensor.run");
        updatesensor.run(stage);
        // Serial.println("INIT Stage: Before transmit.sendSensorData");
        transmit.sendSensorData(stage);
        // Serial.println("INIT Stage: After transmit.sendSensorData");
    }
    if(stage == READY){
        // 텔레메트리 값 읽기 시작
        // Serial.println("READY START");
        updatesensor.run(stage);
        // 읽은 텔레메트리 값 전송 및 저장
        // Serial.println("updateSensor");
        transmit.sendSensorData(stage);
        // Serial.println("Transmit");
        // 만약 가속도 값이 크게 변화하면 상태 ASCENDING으로 바꿈 및 상태 변경 패킷 전송 및 저장
        stagecheck.run();
        // Serial.println("Ready finish");
    }
    else if(stage == ASCENDING){
        // 텔레메트리 값 읽기 시작
        updatesensor.run(stage);    
        // 텔레메트리 값 전송 및 저장
        
        // 읽은 텔레메트리 값 전송 및 저장
        transmit.sendSensorData(stage);
        // PID 제어
        // canard.run();
        stagecheck.run();
    }
    else if(stage == APOGEE){
        // 텔레메트리 값 읽기 시작
        updatesensor.run(stage);
        // 낙하산 사출
        parachute.run();
        delay(100);
        transmit.sendSensorData(stage);

        stagecheck.run();
    }
    else if(stage == DESCENDING){
        // 텔레메트리 값 읽기 시작
        updatesensor.run(stage);
        // 텔레메트리 값 전송 및 저장
        transmit.sendSensorData(stage);
    }
    else if(stage == RETRIEVAL){
        updatesensor.run(stage);
        modem.getModem()->gnss_controller_power_management(0);
        transmit.sendSensorData(stage);
        modem.getModem()->gnss_controller_power_management(1);
    }
    // return;
    delay(100);
}