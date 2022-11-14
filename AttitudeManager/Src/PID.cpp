/**
 * @file PID.cpp
 * @authors Ian Frosst, Anthony Berbari
 */

#include "PID.hpp"
#include "constrain.h"

/***********************************************************************************************************************
 * Code
 **********************************************************************************************************************/

float PIDController::execute(float desired, float actual, float actualRate) {
    float error = desired - actual;
    float derivative;

    // avoid integral windup
    integral = constrain<float>(integral + error, i_max, -i_max);

    // if we are provided with a measured derivative (say from a gyroscope), it
    // is always less noisy to use that than to compute it ourselves.
    if (!std::isnan(actualRate)) {
        derivative = actualRate;
    } else {
        historicalValue[2] = historicalValue[1];
        historicalValue[1] = historicalValue[0];
        historicalValue[0] = actual;

        // Finite difference approximation gets rid of noise much better than
        // first order derivative computation
        derivative = ((3 * historicalValue[0]) - (4 * historicalValue[1]) +
                      (historicalValue[2]));
    }

    float ret =
        constrain<float>((kp * error) + (ki * integral) - (kd * derivative),
                         max_output, min_output);
    return ret;
}
