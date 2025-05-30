#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <math.h>

LOG_MODULE_DECLARE(project_mobile);

static const uint8_t mobile_uuid[16] = {
    0xe2, 0xc5, 0x6d, 0xb5, 0xdf, 0xfb, 0x48, 0xd4,
    0xb0, 0x60, 0xd0, 0xf5, 0xa7, 0x10, 0x96, 0xe3
};

static struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    { .type = BT_DATA_MANUFACTURER_DATA,
      .data_len = 25, .data = NULL },
};

static struct bt_le_adv_param adv_param = {
    .id       = BT_ID_DEFAULT,
    .sid      = 0,
    .secondary_max_skip = 0,
    .options  = BT_LE_ADV_OPT_USE_IDENTITY,
    .interval_min = 0x0020,
    .interval_max = 0x0020,
    .peer = NULL,
};

const char *identify_beacon(const bt_addr_le_t *addr_le)
{
    const char *beacon_strings[][2] = {
        { "A", "F5:75:FE:85:34:67" },
        { "B", "E5:73:87:06:1E:86" },
        { "C", "CA:99:9E:FD:98:B1" },
        { "D", "CB:1B:89:82:FF:FE" },
        { "E", "D4:D2:A0:A4:5C:AC" },
        { "F", "C1:13:27:E9:B7:7C" },
        { "G", "F1:04:48:06:39:A0" },
        { "H", "CA:0C:E0:DB:CE:60" },
        { "I", "CA:99:9E:FD:98:B1" },
        { "J", "F7:0B:21:F1:C8:E1" },
        { "K", "FD:E0:8D:FA:3E:4A" },
        { "L", "EE:32:F7:28:FA:AC" },
        { "M", "EA:E4:EC:C2:0C:18" },
    };
    size_t numBeacons = sizeof(beacon_strings)/sizeof(beacon_strings[0]);
    bt_addr_t beacons[numBeacons];
    for (size_t i = 0; i < numBeacons; ++i) {
        int err = bt_addr_from_str(beacon_strings[i][1], &beacons[i]);
        if (err) {
            LOG_ERR("Failed to parse MAC address\n");
        }
    }
    for (size_t i = 0; i < numBeacons; ++i) {
        if (!bt_addr_cmp(&addr_le->a, &beacons[i])) {
            return beacon_strings[i][0];
        }
    }
    return NULL;
}

void advertise(uint16_t major, uint16_t minor)
{
    static uint8_t beacon_data[25];

    beacon_data[0] = 0x4C; beacon_data[1] = 0x00;
    beacon_data[2] = 0x02; beacon_data[3] = 0x15;
    memcpy(&beacon_data[4], mobile_uuid, 16);

    beacon_data[20] = major >> 8;  beacon_data[21] = major;
    beacon_data[22] = minor >> 8;  beacon_data[23] = minor;
    beacon_data[24] = 0xC5;

    ad[1].data = beacon_data;
    static bool started;
    if (!started) {
        bt_le_adv_start(&adv_param, ad, ARRAY_SIZE(ad), NULL, 0);
        started = true;
    } else {
        bt_le_adv_update_data(ad, ARRAY_SIZE(ad), NULL, 0);
    }
}

void respond(const char *beacon_id, const char *addr_str, int rssi) {
    //LOG_INF("beacon data: %s; %s; %d", beacon_id, addr_str, rssi);
    uint8_t m1 = *beacon_id - 'A';
    uint8_t m2 = -rssi;
    advertise(1, (m1 << 8) | m2);
}

static void device_found(const bt_addr_le_t *addr, int8_t rssi,
    uint8_t adv_type, struct net_buf_simple *ad)
{
    char addr_str[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));

    const char *beacon_id = identify_beacon(addr);
    if (beacon_id) {
        respond(beacon_id, addr_str, rssi);
    }
}

bool init_bluetooth(void) {
    if (bt_enable(NULL) < 0) {
        return false;
    }
    struct bt_le_scan_param scan_params = {
        .type     = BT_HCI_LE_SCAN_PASSIVE,
        .options  = BT_LE_SCAN_OPT_NONE,
        .interval = 0x0010,
        .window   = 0x0010,
    };
    bt_le_scan_start(&scan_params, device_found);
    return true;
}
