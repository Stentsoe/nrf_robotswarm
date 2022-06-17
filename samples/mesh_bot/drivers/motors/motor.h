#pragma once
#include <zephyr.h>
#include <device.h>

typedef int (*drive_continous_t)(const struct device *dev, int32_t power);

typedef int (*set_position_t)(const struct device *dev, int position, int32_t power, bool hold);


struct motor_api
{
    drive_continous_t drive_continous;
    set_position_t set_position;
};

/**
 * @brief Set power of motor
 *
 * @param dev Motor device
 * @param power Power of the motor. Negative values give same power in oposite direction.
 *              Relationship between power parameter and actual output from the motor depends
 *              on the underlying driver
 * @return 0 on success, negative errno code otherwise.
 *         -ENOTSUP if motor does not support continous rotation. 
 *         Other error codes are defined by the underlying driver.
 */
static inline int drive_continous(const struct device *dev, int32_t power)
{
    const struct motor_api *api = (struct motor_api *)dev->api;
    if (api->drive_continous == NULL){
        return -ENOTSUP;
    }
    return api->drive_continous(dev, power);
}

/**
 * @brief Set position of motor.
 * 
 * @param dev Motor device
 * @param position Position the motor shoul be moved to.
 * @param power Power with which to move the motor. Negative values move the motor towards 
 *              Relationship between power parameter and actual output from the motor depends
 *              on the underlying driver
 * @param hold Attempts to hold position when reached if true. Powers off motor if false.
 * @return 0 on success, negative errno code otherwise.
 *         -ENOTSUP if motor does not support moving to a specific position.
 *         -ENOTSUP if hold == true and the motor does not support holding the motor position.
 *         Other error codes are defined by the underlying driver.
 */
static inline int set_position(const struct device *dev, int32_t position, int32_t power, bool hold)
{
    const struct motor_api *api = (struct motor_api *)dev->api;

    if (api->set_position == NULL){
        return -ENOTSUP;
    }

    return api->set_position(dev, position, power, hold);
}