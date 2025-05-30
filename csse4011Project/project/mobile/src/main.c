#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <math.h>

bool init_bluetooth(void);
void advertise(uint16_t major, uint16_t minor);

extern const struct device *gpio_dev;

LOG_MODULE_REGISTER(project_mobile);

#define DEFINE_THREAD(entry) \
	void entry(void *, void *, void *); \
	K_THREAD_DEFINE(id_##entry, 2048, entry, NULL, NULL, NULL, 1, 0, 0)

DEFINE_THREAD(main_thread);

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led), gpios);
static const struct gpio_dt_spec led2 = GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios);
static const struct gpio_dt_spec led3 = GPIO_DT_SPEC_GET(DT_ALIAS(led3), gpios);
//static const struct device *pwm = DEVICE_DT_GET(DT_ALIAS(pwm));

//#define PWM_NODE DT_ALIAS(pwm)
//#define PWM_CHANNEL 0
//#define TONE_FREQ_HZ 200
//#define PERIOD_NS (1000000000UL / TONE_FREQ_HZ)
//#define DUTY_CYCLE (PERIOD_NS / 2)

static const struct device *const p_sensor = DEVICE_DT_GET(DT_ALIAS(p_sensor));
static const struct device *const acc = DEVICE_DT_GET(DT_ALIAS(acc));

#define M_PI 3.141592654

bool init_bluetooth_listen(void);

void handle_message(uint32_t major, uint32_t minor) {
    LOG_INF("message: %u %u", major, minor);
}

bool read_pressure(double *pressure) {
    if (sensor_sample_fetch(p_sensor) < 0) {
        return false;
    }

    struct sensor_value value;
    if (sensor_channel_get(p_sensor, SENSOR_CHAN_PRESS, &value) < 0) {
        return false;
    }
    *pressure = sensor_value_to_double(&value);
    return true;
}

bool read_acc(double *x, double *y, double *z) {
    if (sensor_sample_fetch(acc) < 0) {
        return false;
    }
   
    struct sensor_value value;
    if (sensor_channel_get(acc, SENSOR_CHAN_ACCEL_X, &value) < 0) {
        return false;
    }
    *x = sensor_value_to_double(&value);
    if (sensor_channel_get(acc, SENSOR_CHAN_ACCEL_Y, &value) < 0) {
        return false;
    }
    *y = sensor_value_to_double(&value);
    if (sensor_channel_get(acc, SENSOR_CHAN_ACCEL_Z, &value) < 0) {
        return false;
    }
    *z = sensor_value_to_double(&value);
    return true;
}

bool init_led(void) {
    if (!gpio_is_ready_dt(&led) || gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE) < 0) {
        return false;
    }
    if (!gpio_is_ready_dt(&led2) || gpio_pin_configure_dt(&led2, GPIO_OUTPUT_INACTIVE) < 0) {
        return false;
    }
    if (!gpio_is_ready_dt(&led3) || gpio_pin_configure_dt(&led3, GPIO_OUTPUT_INACTIVE) < 0) {
        return false;
    }
    return true;
}

void led_set(int x, int state) {
    if (x == 0) {
        gpio_pin_set_dt(&led, state);
    } else if (x == 1) {
        gpio_pin_set_dt(&led2, state);
    } else if (x == 2) {
        gpio_pin_set_dt(&led3, state);
    }
}

#define PWM_CHANNEL 0
#define TONE_FREQ_HZ 100
#define PERIOD_NS (1000000000UL / TONE_FREQ_HZ)
#define DUTY_CYCLE (PERIOD_NS / 2)

/*
const struct device *init_pwm(void) {
    if (!device_is_ready(pwm)) {
        return NULL;
    }
    pwm_set(pwm, PWM_CHANNEL, PERIOD_NS, 0, 0);
    return pwm;
}
*/

bool init_pressure(void) {
    return device_is_ready(p_sensor);
}

bool init_acc(void) {
    return device_is_ready(acc);
}

/* PWM stuff

init_pwm();
pwm_set(pwm, PWM_CHANNEL, PERIOD_NS, DUTY_CYCLE, 0);
pwm_set(pwm, PWM_CHANNEL, PERIOD_NS, 0, 0);

*/

#define BUZZ_PIN 4

void main_thread(void *, void *, void *)
{
    if (!init_acc() || !init_pressure() || !init_led() || !init_bluetooth()) {
        return;
    }

    if (!init_bluetooth_listen()) {
        return;
    }
    
    
    //if (!init_pwm()) {
    //    return;
    //}

    led_set(0, 0);
    led_set(1, 1);
    led_set(2, 0);

	int state = 0;
	while (1) {
        double x, y, z;
        if (!read_acc(&x, &y, &z)) {
            LOG_ERR("failed to read accelerometer");
        }

        double pressure;
        if (!read_pressure(&pressure)) {
            LOG_ERR("failed to read pressure");
        }

        double angle = atan2(pow(x*x+y*y, 0.5), z) / M_PI * 180;
        advertise(2, (uint16_t) (angle * 100));

        k_msleep(20);

        advertise(3, (uint16_t) ((pressure - 100) * 10000));

        if (state % 10 == 0) {
            LOG_INF("did loop");
        }

		for (int i = 0; i < 3; ++i) {
// 			led_set(i, i == (state / 30) % 3);
		}
		k_msleep(20);
		state = (state + 1) % 90;
	}
}
