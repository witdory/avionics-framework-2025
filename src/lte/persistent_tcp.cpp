#include "persistent_tcp.h"

PersistentTCP::PersistentTCP(Modem* modem, const char* server_ip, int port)
  : _modem(modem), _connID(1), _port(port), _server_ip(server_ip), _connected(false) {}

bool PersistentTCP::connect() {
    Serial.println("Attempting TCP connection...");
    auto rc = _modem->getModem()->socket_configuration(_connID, 1);  // PDP context 1
    Serial.print("Socket configuration rc: "); Serial.println(rc);
    if (rc != me310::ME310::RETURN_VALID) {
        Serial.println("Socket configuration failed.");
        return false;
    }

    rc = _modem->getModem()->socket_dial(_connID, 0, _port, _server_ip, 0, 0, 1, 0, 0, me310::ME310::TOUT_1MIN);
    Serial.print("Socket dial rc: "); Serial.println(rc);
    _connected = (rc == me310::ME310::RETURN_VALID);
    if (!_connected) {
        Serial.println("Socket dial failed.");
    }
    return _connected;
}

bool PersistentTCP::send(const char* data) {
    if (!_connected) {
        return false;
    }
    const int MAX_SEND_RETRIES = 3;
    const int SEND_RETRY_DELAY_MS = 1000; // 1 second

    for (int i = 0; i < MAX_SEND_RETRIES; ++i) {
        auto rc = _modem->getModem()->socket_send_data_command_mode_extended(
            _connID, strlen(data), const_cast<char*>(data), 1, me310::ME310::TOUT_30SEC);
        if (rc == me310::ME310::RETURN_VALID) {
            return true; // Success
        }
        delay(SEND_RETRY_DELAY_MS);
    }
    _connected = false; // All retries failed
    return false;
}

void PersistentTCP::close() {
    _modem->getModem()->socket_listen(_connID, 0, _port);
    _connected = false;
}

bool PersistentTCP::isConnected() {
    return _connected;
}

String PersistentTCP::receive() {
    const int MAX_RECEIVE_RETRIES = 3;
    const int RECEIVE_RETRY_DELAY_MS = 1000; // 1 second

    for (int i = 0; i < MAX_RECEIVE_RETRIES; ++i) {
        Serial.print("Receive attempt "); Serial.print(i + 1);
        Serial.println("...");

        // Check socket status for pending data before attempting to receive
        me310::ME310::return_t status_rc = _modem->getModem()->socket_status(_connID, me310::ME310::TOUT_10SEC); // Use a reasonable timeout
        Serial.print("Socket status rc: "); Serial.println(status_rc);

        if (status_rc == me310::ME310::RETURN_VALID) {
            // Check if there is pending data (e.g., status code indicates data available)
            // This part might need more specific logic based on ME310::socket_status return values
            // For now, assuming RETURN_VALID means there might be data or connection is active
            me310::ME310::return_t rc = _modem->getModem()->socket_receive_data_command_mode(_connID, 256, 0, me310::ME310::TOUT_3SEC);
            Serial.print("socket_receive_data_command_mode rc: "); Serial.println(rc);
            if (rc == me310::ME310::RETURN_VALID) {
                String receivedData = (String)_modem->getModem()->buffer_cstr_raw();
                return receivedData;
            }
        } else if (status_rc == me310::ME310::RETURN_ERROR) { // Changed from RETURN_TIMEOUT
            Serial.println("Socket status error or timeout. No pending data.");
        } else {
            Serial.println("Socket status failed. Connection might be down.");
            _connected = false; // Connection likely lost
            return ""; // Exit early if connection is bad
        }

        if (i < MAX_RECEIVE_RETRIES - 1) {
            Serial.println("Delaying for retry...");
            delay(RECEIVE_RETRY_DELAY_MS);
        }
    }
    _connected = false; // All retries failed
    return "";
}