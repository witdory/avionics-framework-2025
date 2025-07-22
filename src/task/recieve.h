#pragma once
#include "../modules.h"

/*
필요 기능
1. 버퍼에 있는 데이터를 패킷 단위로 짤라서 보관한다.
2. 오류가 있는 패킷이나 중간에 노이즈가 낀 패킷은 버린다.
3. 받은 패킷의 데이터만 뽑아서 리턴한다.
*/

class Recieve : public Task{
public:
    Recieve(){}
    // 향후 HTTP GET 등으로 명령 수신 구현 가능
};