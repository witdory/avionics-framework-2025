#include <Arduino.h>

#include "modem.h"


#include "lib/sensor.h"
#include "lib/task.h"
#include "lib/stage.h"
#include "lib/queue.h"

#include "sensor/altemeter.h"
#include "sensor/motor.h"
#include "sensor/logger.h"
#include "sensor/imu.h"
#include "sensor/gps.h"

#include"task/updatesensor.h"
#include"task/parachute.h"
#include"task/recieve.h"
#include"task/stagecheck.h"
#include "lte/persistent_tcp.h"
#include "lte/HttpClient.h"
