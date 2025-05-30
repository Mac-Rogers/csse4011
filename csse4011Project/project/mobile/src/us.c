#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>

void advertise(uint16_t major, uint16_t minor);

// Ultrasonic sensor connections
#define TRIGGER_PIN 2  // P0.02
#define ECHO_PIN    3  // P0.03

#define PULSE_TIMEOUT_US 30000  // 30 ms timeout (max ~5m range)

// Onboard LED (via SX1509B expander)
//static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

const struct device *gpio_dev;

static volatile uint32_t echo_start_time;
static volatile uint32_t echo_end_time;
static volatile bool echo_received = false;

bool led_set(int, int);

#define DEFINE_THREAD(entry) \
	void entry(void *, void *, void *); \
	K_THREAD_DEFINE(id_##entry, 2048, entry, NULL, NULL, NULL, 1, 0, 0)

DEFINE_THREAD(us_thread);

static void echo_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    if (gpio_pin_get(dev, ECHO_PIN)) {
        echo_start_time = k_cycle_get_32();
    } else {
        echo_end_time = k_cycle_get_32();
        echo_received = true;
    }
}

#define BUZZ_PIN 4

void us_thread(void *, void *, void *)
{
    const struct device *gpio_dev = DEVICE_DT_GET(DT_NODELABEL(gpio0));
    gpio_pin_configure(gpio_dev, 2, GPIO_OUTPUT_INACTIVE);

    struct gpio_callback echo_cb_data;
    uint32_t cycle_freq = sys_clock_hw_cycles_per_sec();

    // Configure echo pin as input with interrupt
    gpio_pin_configure(gpio_dev, ECHO_PIN, GPIO_INPUT);
    gpio_init_callback(&echo_cb_data, echo_callback, BIT(ECHO_PIN));
    gpio_add_callback(gpio_dev, &echo_cb_data);
    gpio_pin_interrupt_configure(gpio_dev, ECHO_PIN, GPIO_INT_EDGE_BOTH);

//    const struct device *gpio_dev = DEVICE_DT_GET(DT_NODELABEL(gpio0));
    gpio_pin_configure(gpio_dev, BUZZ_PIN, GPIO_OUTPUT_INACTIVE);
    gpio_pin_set(gpio_dev, BUZZ_PIN, 0);

    // Configure onboard LED as output
//    gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);

    while (1) {
        // Reset echo tracking
        echo_received = false;

        // Send 10µs trigger pulse
        gpio_pin_set(gpio_dev, TRIGGER_PIN, 1);
        k_busy_wait(10);
        gpio_pin_set(gpio_dev, TRIGGER_PIN, 0);

        // Wait for echo with timeout
        int64_t start_time = k_uptime_get();
        while (!echo_received && (k_uptime_get() - start_time < PULSE_TIMEOUT_US / 1000)) {
            k_msleep(1);
        }

        float distance_cm = -1.0f;

        if (echo_received) {
            uint32_t duration_cycles = echo_end_time - echo_start_time;
            uint32_t duration_us = (duration_cycles * 1000000U) / cycle_freq;
            distance_cm = (duration_us * 0.0343f) / 2.0f;
        }

        advertise(4, (uint16_t) (distance_cm));            

        if (distance_cm < 10 && distance_cm >= 1) {
            gpio_pin_set(gpio_dev, BUZZ_PIN, 1);
            led_set(0, 1);
            led_set(1, 0);
            led_set(2, 0);
        } else {
            gpio_pin_set(gpio_dev, BUZZ_PIN, 0);
            led_set(0, 0);
            led_set(1, 1);
            led_set(2, 0);
        }

        k_msleep(100);
    }
}