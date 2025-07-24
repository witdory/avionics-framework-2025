#include <SoftwareSerial.h>

// 핀 설정
const byte rxPin = 5;
const byte txPin = 6;
SoftwareSerial mpuSerial(rxPin, txPin);

// 데이터 처리용 변수
float acc[3], gyro[3], angle[3];

// 캘리브레이션을 위한 오차 보정값 변수
float acc_offset[3] = {0.0, 0.0, 0.0}; 

// 데이터 수신을 위한 버퍼 및 상태 변수
byte buffer[11];
byte bufferIndex = 0;

void setup() {
  Serial.begin(115200);        // PC <-> 아두이노 시리얼 (115200)
  mpuSerial.begin(115200);     // 아두이노 <-> 센서 UART (115200)

  Serial.println("MPU-6050 가속도 센서 캘리브레이션을 시작합니다.");
  Serial.println("센서를 평평한 바닥에 두고 5초간 움직이지 마세요...");
  delay(1000); // 사용자 준비 시간

  calibrateAccelerometer();    // 캘리브레이션 함수 호출

  Serial.println("캘리브레이션 완료!");
  Serial.println("보정된 가속도 값을 출력합니다 (X, Y, Z)");
  Serial.println("------------------------------------");
}


void loop() {
  // 수신된 데이터가 있다면 처리
  while (mpuSerial.available()) {
    byte incomingByte = mpuSerial.read();

    if (bufferIndex == 0 && incomingByte == 0x55) {
      buffer[bufferIndex++] = incomingByte;
    } else if (bufferIndex > 0 && bufferIndex < 11) {
      buffer[bufferIndex++] = incomingByte;
      if (bufferIndex >= 11) {
        if (verifyChecksum(buffer)) {
          parseData(buffer);
        }
        bufferIndex = 0; // 다음 패킷을 위해 인덱스 초기화
      }
    }
  }
}

// 캘리브레이션을 수행하는 함수
void calibrateAccelerometer() {
  float acc_sum[3] = {0.0, 0.0, 0.0};
  int sample_count = 500; // 오차 측정을 위한 샘플 수

  for (int i = 0; i < sample_count; i++) {
    // 데이터 패킷이 들어올 때까지 대기
    while (mpuSerial.available() < 11) {}

    // 11바이트 패킷을 읽고 파싱하여 가속도 값만 가져옴
    byte temp_buffer[11];
    for (int j = 0; j < 11; j++) {
      temp_buffer[j] = mpuSerial.read();
    }

    if (temp_buffer[0] == 0x55 && temp_buffer[1] == 0x51) {
      // 체크섬 검증
      byte sum = 0;
      for (int k = 0; k < 10; k++) sum += temp_buffer[k];
      if (sum == temp_buffer[10]) {
          acc_sum[0] += (short)(temp_buffer[3] << 8 | temp_buffer[2]);
          acc_sum[1] += (short)(temp_buffer[5] << 8 | temp_buffer[4]);
          acc_sum[2] += (short)(temp_buffer[7] << 8 | temp_buffer[6]);
      } else {
        i--; // 잘못된 패킷이면 샘플 카운트에서 제외
      }
    } else {
      i--; // 가속도 패킷이 아니면 샘플 카운트에서 제외
    }
    delay(2);
  }

  // 평균을 내어 오차(offset) 계산
  acc_offset[0] = acc_sum[0] / sample_count;
  acc_offset[1] = acc_sum[1] / sample_count;
  // Z축은 중력(1g)의 영향을 받으므로, 1g에 해당하는 원시값(2048)을 빼서 오차를 계산
  // (32768.0 / 16.0 = 2048)
  acc_offset[2] = acc_sum[2] / sample_count - 2048.0; 
}


// 체크섬 검증 함수
bool verifyChecksum(byte* data) {
  byte sum = 0;
  for (int i = 0; i < 10; i++) {
    sum += data[i];
  }
  return sum == data[10];
}

// 완성된 패킷(11바이트)을 인자로 받아 파싱하는 함수
void parseData(byte* data) {
  if (data[1] == 0x51) { // 가속도 데이터일 때만 처리
    // 원시 데이터(Raw data)를 읽어옴
    float raw_acc[3];
    raw_acc[0] = (short)(data[3] << 8 | data[2]);
    raw_acc[1] = (short)(data[5] << 8 | data[4]);
    raw_acc[2] = (short)(data[7] << 8 | data[6]);

    // **캘리브레이션 오차 값을 빼주고, g 단위로 변환**
    acc[0] = (raw_acc[0] - acc_offset[0]) / 32768.0 * 16.0;
    acc[1] = (raw_acc[1] - acc_offset[1]) / 32768.0 * 16.0;
    acc[2] = (raw_acc[2] - acc_offset[2]) / 32768.0 * 16.0;
    
    // 시리얼 플로터를 위해 X, Y, Z 가속도 값을 쉼표로 구분하여 출력
    Serial.print(acc[0]);
    Serial.print(",");
    Serial.print(acc[1]);
    Serial.print(",");
    Serial.println(acc[2]);
  }
}