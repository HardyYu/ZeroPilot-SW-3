/*
 * PID.hpp
 *
 *  Created on: May 27, 2022
 *      Author: Anthony (anni) Luo
 */

#ifndef PID_HPP_
#define PID_HPP_

#include <cmath>
#include <cstdint>

/***********************************************************************************************************************
 * Prototypes
 **********************************************************************************************************************/

class PIDController {
   public:
    /**
     * Initialises the Pid object.
     * @param[in]	_kp 	The proportional gain.
     * @param[in]	_ki 	The integral gain.
     * @param[in]	_kd  	The derivative gain.
     * @param[in]	_i_max	Max value the integral should ever be allowed to take
     * on.
     * @param[in]	_min_output		The minimum value that can be output, if
     * computations return smaller, the output will be set to this value.
     * @param[in]	_max_output		The maximum value that can be output, if
     * computations return larger, the output will be set to this value.
     */
    PIDController(float _kp, float _ki, float _kd, float _i_max,
                  float _min_output, float _max_output)
        : kp(_kp),
          kd(_kd),
          ki(_ki),
          i_max(_i_max),
          min_output(_min_output),
          max_output(_max_output) {}

    /**
     * Executes a PID computation.
     * The PID algorithm uses the derivative of the actual values, rather than
     * the derivatives of the error. That makes it immune to sudden changes in
     * commanded set points. THIS METHOD MUST BE CALLED AT A REGULAR INTERVAL!!!
     * UNDEFINED BEHAVIOUR WILL OCCUR OTHERWISE. Will use the measured
     * derivative if provided with one. Otherwise, will compute the derivative
     * from the given measurements.
     * @param[in]	desired 	The point we wish to reach.
     * @param[in]	actual 		The current point.
     * @param[in]	actualRate  The current measured derivative (This parameter is
     * optional).
     * @return					The result of the PID
     * computation.
     */
    float execute(float desired, float actual,
                  float actualRate = std::nanf(""));
    float execute_p(float desired, float actual);

   private:
    const float kp, kd, ki;
    const float i_max;
    const float min_output;
    const float max_output;

    float integral = 0.0f;
    float historicalValue[3] = {0.0f};
};

#endif /* PID_HPP_ */
