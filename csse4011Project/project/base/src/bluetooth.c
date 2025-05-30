#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/logging/log.h>
#include <math.h>

LOG_MODULE_DECLARE(project_base);

void handle_message(uint16_t major, uint16_t minor);

static const uint8_t mobile_uuid[16] = {
    0xe2, 0xc5, 0x6d, 0xb5, 0xdf, 0xfb, 0x48, 0xd4,
    0xb0, 0x60, 0xd0, 0xf5, 0xa7, 0x10, 0x96, 0xe3
};

#define NUM_BEACONS 13
typedef struct { float x; float y; } BeaconPos;

static const BeaconPos beacon_pos[NUM_BEACONS] = {
    {0.0f, 0.0f},   /* A */
    {1.5f, 0.0f},   /* B */
    {3.0f, 0.0f},   /* C */
    {3.0f, 2.0f},   /* D */
    {3.0f, 4.0f},   /* E */
    {1.5f, 4.0f},   /* F */
    {0.0f, 4.0f},   /* G */
    {0.0f, 2.0f},   /* H */
    {4.5f, 0.0f},   /* I */
    {6.0f, 0.0f},   /* J */
    {6.0f, 2.0f},   /* K */
    {6.0f, 4.0f},   /* L */
    {4.5f, 4.0f},   /* M */
};

static float last_dist[NUM_BEACONS] = {
    [0 ... NUM_BEACONS-1] = -1.0f
};

static float x_hat = 3.0f, y_hat = 2.0f;
static float P_x = 25.0f, P_y = 25.0f;
static const float Q = 0.05f;
static const float R = 0.25f;

static inline float minor_to_distance(uint16_t minor)
{
    return powf(10.0f, (((float)minor) - 59.0f) / 20.0f);
}

static void kalman_update(float z_x, float z_y)
{
    P_x += Q;
    P_y += Q;

    float Kx = P_x / (P_x + R);
    float Ky = P_y / (P_y + R);

    x_hat += Kx * (z_x - x_hat);
    y_hat += Ky * (z_y - y_hat);

    P_x *= (1.0f - Kx);
    P_y *= (1.0f - Ky);

    printk("(coords) %.2lf %.2lf\n", (double) x_hat, (double) y_hat);
}

static bool handle_distance(uint16_t beacon_idx, uint16_t minor)
{
    if (beacon_idx != 1) {
        return false;
    }
    beacon_idx = (minor >> 8);
    minor = minor % 256;

    if (beacon_idx >= NUM_BEACONS)
        return false;

    last_dist[beacon_idx] = minor_to_distance(minor);

    int valid = 0;
    for (int i = 0; i < NUM_BEACONS; ++i)
        if (last_dist[i] > 0) ++valid;
    if (valid < 3)
        return false;

    float sum_w = 0.0f, est_x = 0.0f, est_y = 0.0f;
    for (int i = 0; i < NUM_BEACONS; ++i) {
        float d = last_dist[i];
        if (d <= 0) continue;
        float w = 1.0f / (d * d + 1e-3f);
        sum_w += w;
        est_x += w * beacon_pos[i].x;
        est_y += w * beacon_pos[i].y;
    }
    if (sum_w > 0.0f)
        kalman_update(est_x / sum_w, est_y / sum_w);
    return true;
}

static bool parse_mobile(const uint8_t *data, uint8_t len,
                         uint16_t *p_major, uint16_t *p_minor,
                         char *identifier)
{
    if (len < 25) return false;

    if (data[0] != 0x4C || data[1] != 0x00 || data[2] != 0x02 || data[3] != 0x15)
        return false;

    if (!memcmp(&data[4], mobile_uuid, 16)) {
        strcpy(identifier, "mobile");
    } else {
        return false;
    }

    *p_major = (data[20] << 8) | data[21];
    *p_minor = (data[22] << 8) | data[23];
    return true;
}

static void device_found(const bt_addr_le_t *addr, int8_t rssi,
                         uint8_t adv_type, struct net_buf_simple *ad)
{
    struct net_buf_simple ad_copy = *ad;
    while (ad_copy.len > 1) {
        uint8_t len = net_buf_simple_pull_u8(&ad_copy);
        if (!len || len > ad_copy.len) break;

        uint8_t ad_type = net_buf_simple_pull_u8(&ad_copy);
        const uint8_t *data = ad_copy.data;

        if (ad_type == BT_DATA_MANUFACTURER_DATA && len >= 2) {
            uint16_t major, minor; char id[16];
            if (parse_mobile(data, len - 1, &major, &minor, id)) {
                if (!strcmp(id, "mobile"))
                    if (!handle_distance(major, minor)) {
                        handle_message(major, minor);
                    }
            }
        }
        net_buf_simple_pull(&ad_copy, len - 1);
    }
}

bool init_bluetooth(void) {
    if (bt_enable(NULL)) {
        return false;
    }

    const struct bt_le_scan_param p = {
        .type     = BT_HCI_LE_SCAN_PASSIVE,
        .options  = BT_LE_SCAN_OPT_NONE,
        .interval = 0x0010,
        .window   = 0x0010,
    };
    bt_le_scan_start(&p, device_found);
    return true;
}
