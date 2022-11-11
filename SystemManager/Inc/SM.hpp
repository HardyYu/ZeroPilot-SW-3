/*
 * SM.hpp
 *
 * System Manager Header
 *
 * Created on: Nov 7, 2022
 * Author(s): Gordon Fountain
 */

#ifndef ZPSW3_SM_HPP
#define ZPSW3_SM_HPP

#include <stdint.h>
// #include "LOS_Link.hpp"
// #include "LOS_Actuators.hpp"
// #include "AM_StateManager.hpp"

class SystemState;

namespace SM {
    // Enumerates the current drone status
    enum _System_Manager_Drone_Operation_Mode{BOOT, DISARMED, GROUND_OPS, FLIGHT, FATAL_FAILURE = -1};

    enum _System_Manager_Cycle_Status {COMPLETED_CYCLE = 0, IN_CYCLE, FAILURE_MODE};
}

class SystemManager {
public:
    SystemManager();
    void execute();
    void setState(SystemState& newState);
private:
    SystemState* currentState;
    SM::_System_Manager_Cycle_Status status;
    SystemManager();

    // TODO PM, AM, TM instances here

    // TODO LOS actuator instance here

    // TODO Bulk message from telemetry stored here

    // TODO Responses from SF/AM stored here to be merged and sent to Telemetry

    inputs_to_AM_t* AM_Waypoints;
};

#endif //ZPSW3_SM_HPP