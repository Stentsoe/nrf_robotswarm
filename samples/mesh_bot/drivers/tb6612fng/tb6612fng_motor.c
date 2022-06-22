
#include <devicetree.h>
#include <device.h>
#include <drivers/gpio.h>
#include <drivers/pwm.h>
#include "../motors/motor.h"

#define DT_DRV_COMPAT toshiba_tb6612fng_motor
#define TB6612FNG_MOTOR_INIT_PRIORITY 60

#include <logging/log.h>
LOG_MODULE_REGISTER(tb6612fng_motor_driver, CONFIG_TB6612FNG_MOTOR_DRIVER_LOG_LEVEL);

struct motor_data
{
};

struct motor_conf
{
    struct gpio_dt_spec gpio1;
    struct gpio_dt_spec gpio2;
    struct pwm_dt_spec pwm;
};

static int _drive_continous(const struct device *dev, int32_t power)
{
    struct motor_conf *conf = (struct motor_conf *)dev->config;

    if (conf->gpio1.port != NULL && conf->gpio2.port != NULL)
    {
        if (power > 0)
        { // CW
            gpio_pin_set_dt(&conf->gpio1, 1);
            gpio_pin_set_dt(&conf->gpio2, 0);
        }
        else if (power < 0)
        { // CCW
            gpio_pin_set_dt(&conf->gpio1, 0);
            gpio_pin_set_dt(&conf->gpio2, 1);
        } else {
            gpio_pin_set_dt(&conf->gpio1, 0);
            gpio_pin_set_dt(&conf->gpio2, 0);
        }
    }
    else if (power < 0)
    {
        LOG_WRN("Negative power %d given but driver has no direction control GPIOs", power);
    }

    int32_t abs_power = power >= 0 ? power : -power;

    int err = pwm_set_pulse_dt(&conf->pwm, abs_power);
    if (err) {
        LOG_ERR("Failed to set PWM pulse: Error %d", err);
        return err;
    }

    LOG_DBG("Setting power on motor %s to %d", dev->name, abs_power);
    return 0;
}

struct motor_api api = {
    .drive_continous = _drive_continous,
    .set_position = NULL,
};

static int init_gpio(const struct device *dev)
{
    struct motor_conf *conf = (struct motor_conf *)dev->config;
    int err = 0;

    if (conf->gpio1.port != NULL)
    {
        if (!device_is_ready(conf->gpio1.port))
        {
            LOG_ERR("Gpio1, %s, is not ready", conf->gpio1.port->name);
            return -ENODEV;
        }

        err = gpio_pin_configure_dt(&conf->gpio1, GPIO_OUTPUT);
        if (err)
        {
            LOG_ERR("Failed to configure gpio1");
            return err;
        }
    }

    if (conf->gpio2.port != NULL)
    {
        if (!device_is_ready(conf->gpio2.port))
        {
            LOG_ERR("Gpio2, %s, is not ready", conf->gpio2.port->name);
            return -ENODEV;
        }

        err = gpio_pin_configure_dt(&conf->gpio2, GPIO_OUTPUT);
        if (err)
        {
            LOG_ERR("Failed to configure gpio2");
            return err;
        }
    }

    return 0;
}

static int init_pwm(const struct device *dev)
{
    struct motor_conf *conf = (struct motor_conf *)dev->config;
    if (!device_is_ready(conf->pwm.dev))
    {
        LOG_ERR("PWM device not found or not ready");
        return -ENODEV;
    }
    LOG_DBG("Channel %d on PWM %s initialized for motor %s", conf->pwm.channel, conf->pwm.dev->name, dev->name);
    return 0;
}

static int init_motor(const struct device *dev)
{
    int err = init_gpio(dev);
    if (err)
    {
        LOG_ERR("Error while initializig gpio: %d", err);
        return err;
    }
    err = init_pwm(dev);
    if (err)
    {
        LOG_ERR("Error while initializig pwm: %d", err);
        return err;
    }
    LOG_DBG("Motor %s initialized", dev->name);
    return 0;
}

#define INIT_TB6612FNG_MOTOR(inst)                                  \
    static struct motor_conf conf_##inst = {                        \
        .gpio1 = GPIO_DT_SPEC_INST_GET_OR(inst, input1_gpios, {0}), \
        .gpio2 = GPIO_DT_SPEC_INST_GET_OR(inst, input2_gpios, {0}), \
        .pwm = PWM_DT_SPEC_GET(DT_INST(inst, DT_DRV_COMPAT)),       \
    };                                                              \
    static struct motor_data data_##inst = {};                      \
    DEVICE_DT_INST_DEFINE(                                          \
        inst,                                                       \
        init_motor,                                                 \
        NULL,                                                       \
        &data_##inst,                                               \
        &conf_##inst,                                               \
        POST_KERNEL,                                                \
        TB6612FNG_MOTOR_INIT_PRIORITY,                              \
        &api);

DT_INST_FOREACH_STATUS_OKAY(INIT_TB6612FNG_MOTOR)