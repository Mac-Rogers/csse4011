#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>
#include <math.h>

LOG_MODULE_REGISTER(project_base);

bool init_bluetooth(void);

#define DEFINE_THREAD(entry) \
	void entry(void *, void *, void *); \
	K_THREAD_DEFINE(id_##entry, 2048, entry, NULL, NULL, NULL, 1, 0, 0)

DEFINE_THREAD(main_thread);

static int cmd_flash(const struct shell *, size_t, char **);
SHELL_CMD_REGISTER(flash, NULL, "flash the LED", cmd_flash);

static int cmd_flash(const struct shell *sh, size_t argc, char **argv) {
    shell_print(sh, "would flash the LED");
    return 0;
}

double init_pressures[4];
int num_init_pressures;

void handle_message(uint16_t major, uint16_t minor) {
    if (major == 2) {
        double angle = (double) minor / 100;
        angle = 180.0 - angle;
        printk("(tilt) %lf\n", angle);
    } else if (major == 3) {
        double pressure = (double) minor / 10000 + 100;
        if (num_init_pressures < 4) {
            init_pressures[num_init_pressures++] = pressure;
        }
        double sum = 0.0;
        for (int i = 0; i < num_init_pressures; ++i) {
            sum += init_pressures[i];
        }
        double init_pressure = sum / num_init_pressures;
        double height = (init_pressure - pressure) * 100 * 86;
        printk("(pressure) %lf\n", pressure);
        printk("(height) %lf\n", height);
    } else if (major == 4) {
        double dist = minor;
        printk("(distance) %lf\n", dist);
    }
}

void advertise(uint16_t major, uint16_t minor);
bool init_bluetooth_transmit(void);

void main_thread(void *, void *, void *) {
    if (!init_bluetooth() || !init_bluetooth_transmit()) {
        LOG_ERR("failed to init bluetooth");
        return;
    }

    uint32_t counter = 0;
    while (1) {
        advertise(0, counter++);
        k_msleep(50);
    }
}