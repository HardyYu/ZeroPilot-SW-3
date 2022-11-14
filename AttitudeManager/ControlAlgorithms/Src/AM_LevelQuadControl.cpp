#include "AM_LevelQuadControl.hpp"
#include "PID.hpp"

// #include "LOS_SensorFusion.hpp" Whatever this is it will be needed
typedef struct {
    float roll, pitch, yaw;              // Degrees. Yaw of 180 is north.
    float rollRate, pitchRate, yawRate;  // Degrees/second
    float airspeed;                      // m/s
    float altitude;                      // m
    float rateOfClimb;                   // m/s
    long double latitude;                // Decimal degrees
    float latitudeSpeed;                 // m/s
    long double longitude;               // Decimal degrees
    float longitudeSpeed;                // m/s
    double track;                        // Degrees. Track of 0 is north.
    float groundSpeed;                   // m/s
    double heading;                      // Degrees. Heading of 0 is north.
} SFOutput_t;

namespace AM {

std::vector<ActuatorOutput> LevelQuadControl::runControlsAlgorithm(
    const AttitudeManagerInput &instructions) const {
    // Get current attitude from sensorfusion
    // TODO: This needs to be retrieved from LOS
    SFOutput_t currentAttitude{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    // convert instructions into level mode quad instructions
    float targetPitch = instructions.x_dir * maxPitch;
    float targetRoll = instructions.y_dir * maxRoll;
    float targetYaw = currentAttitude.yaw + instructions.heading;
    float targetAltitude = instructions.z_dir;

    PIDController pid_pitch{pitch_kp,     pitch_ki,     pitch_kd,
                            max_i_windup, -pid_abs_max, pid_abs_max};
    PIDController pid_roll{roll_kp,      roll_ki,      roll_kd,
                           max_i_windup, -pid_abs_max, pid_abs_max};
    PIDController pid_yaw{yaw_kp,       yaw_ki,       yaw_kd,
                          max_i_windup, -pid_abs_max, pid_abs_max};
    PIDController pid_altitude{altitude_kp,  altitude_ki,  altitude_kd,
                               max_i_windup, -pid_abs_max, pid_abs_max};

    // get PID result
    float pitch = pid_pitch.execute(targetPitch, currentAttitude.pitch,
                                    currentAttitude.pitchRate);
    float roll = pid_roll.execute(targetRoll, currentAttitude.roll,
                                  currentAttitude.rollRate);
    float yaw = pid_yaw.execute(targetYaw, currentAttitude.yawRate);
    float altitude =
        pid_altitude.execute(targetAltitude, currentAttitude.rateOfClimb);

    // mix the PID's.
    float frontRightOutput =
        mixPIDs(configs[FrontRight].stateMix, roll, pitch, yaw, altitude);
    float frontLeftOutput =
        mixPIDs(configs[FrontLeft].stateMix, roll, pitch, yaw, altitude);
    float backLeftOutput =
        mixPIDs(configs[BackLeft].stateMix, roll, pitch, yaw, altitude);
    float backRightOutput =
        mixPIDs(configs[BackRight].stateMix, roll, pitch, yaw, altitude);

    // return output
    return std::vector<ActuatorOutput>{
        {configs[FrontRight].channel, frontRightOutput},
        {configs[FrontLeft].channel, frontLeftOutput},
        {configs[BackRight].channel, backRightOutput},
        {configs[BackLeft].channel, backLeftOutput}};
}

float LevelQuadControl::mixPIDs(StateMix actuator, float roll, float pitch,
                                float yaw, float altitude) const {
    return constrain<float>(
        (actuator.pitch * pitch + actuator.roll * roll + actuator.yaw * yaw +
         actuator.velocity_z * altitude),
        100, 0);
}

}  // namespace AM