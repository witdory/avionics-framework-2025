#include "persistent_tcp.h"

PersistentTCP::PersistentTCP(Modem* modem, const char* server_ip, int port)
  : _modem(modem), _connID(1), _port(port), _server_ip(server_ip), _connected(false) {}

bool PersistentTCP::connect() {
    auto rc = _modem->getModem()->socket_configuration(_connID, 1);  // PDP context 1
    if (rc != me310::ME310::RETURN_VALID) return false;

    rc = _modem->getModem()->socket_dial(_connID, 0, _port, _server_ip, 0, 0, 1, 0, 0, me310::ME310::TOUT_1MIN);
    _connected = (rc == me310::ME310::RETURN_VALID);
    return _connected;
}

bool PersistentTCP::send(const char* data) {
    if (!_connected) {
        return false;
    }
    auto rc = _modem->getModem()->socket_send_data_command_mode_extended(
        _connID, strlen(data), const_cast<char*>(data), 1, me310::ME310::TOUT_30SEC);
    if (rc != me310::ME310::RETURN_VALID) {
        _connected = false; // 오류 시 연결상태 해제
        return false;
    }
    return true;
}

void PersistentTCP::close() {
    Serial.println("PersistentTCP: Closing connection.");
    _modem->getModem()->socket_listen(_connID, 0, _port);
    _connected = false;
    Serial.println("PersistentTCP: Connection closed.");
}

bool PersistentTCP::isConnected() {
    return _connected;
}

String PersistentTCP::receive() {
    me310::ME310::return_t rc = _modem->getModem()->socket_receive_data_command_mode(_connID, 256, 0, me310::ME310::TOUT_10SEC);
    if (rc == me310::ME310::RETURN_VALID) {
        String receivedData = (String)_modem->getModem()->buffer_cstr_raw();
        return receivedData;
    }
    return "";
}
