#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/logging/log.h>
#include <math.h>

void handle_message(uint32_t major, uint32_t minor);

static const uint8_t base_uuid[16] = {
    0xe2, 0xc5, 0x6d, 0xb5, 0xdf, 0xfb, 0x48, 0xd4,
    0xb0, 0x60, 0xd0, 0xf5, 0xa7, 0x10, 0x96, 0xe4
};

bool init_bluetooth_listen(void) {
    return true;
}