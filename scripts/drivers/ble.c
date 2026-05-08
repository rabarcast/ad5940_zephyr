#include "ble.h"

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/conn.h>
#include <string.h>
#include <zephyr/sys/printk.h>

/* ================= CONNECTION ================= */

static struct bt_conn *current_conn = NULL;
static bool notify_enabled = false;

/* ================= BUFFERS ================= */

static uint8_t notify_buf[32];
static uint8_t rx_buf[64];

/* ================= UUIDs ================= */

#define BT_UUID_FALL_SERVICE_VAL \
    BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef0)

#define BT_UUID_FALL_CHAR_NOTIFY_VAL \
    BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef1)

#define BT_UUID_FALL_CHAR_WRITE_VAL \
    BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef2)

static struct bt_uuid_128 fall_service_uuid =
    BT_UUID_INIT_128(BT_UUID_FALL_SERVICE_VAL);

static struct bt_uuid_128 fall_char_notify_uuid =
    BT_UUID_INIT_128(BT_UUID_FALL_CHAR_NOTIFY_VAL);

static struct bt_uuid_128 fall_char_write_uuid =
    BT_UUID_INIT_128(BT_UUID_FALL_CHAR_WRITE_VAL);

/* ================= ADVERTISING ================= */

static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS,
                  (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
};

static const struct bt_data sd[] = {
    BT_DATA(BT_DATA_NAME_COMPLETE, "FALL_SENSOR", 11),
};

/* ================= FORWARD DECL ================= */

static void start_advertising(void);
static int ble_send_test(void);

/* ================= CCC ================= */

static void ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
    ARG_UNUSED(attr);

    notify_enabled = (value == BT_GATT_CCC_NOTIFY);

    printk("Notify: %s\n", notify_enabled ? "ON" : "OFF");

    if (notify_enabled) {
        ble_send_test();
    }
}

/* ================= WRITE CALLBACK (GPS FROM MOBILE) ================= */

static ssize_t write_rx(struct bt_conn *conn,
                        const struct bt_gatt_attr *attr,
                        const void *buf,
                        uint16_t len,
                        uint16_t offset,
                        uint8_t flags)
{
    ARG_UNUSED(conn);
    ARG_UNUSED(attr);
    ARG_UNUSED(flags);

    if (offset + len > sizeof(rx_buf)) {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
    }

    memcpy(rx_buf + offset, buf, len);
    rx_buf[offset + len] = '\0';

    printk("📍 GPS recibido: %s\n", rx_buf);

    return len;
}

/* ================= SERVICE ================= */

BT_GATT_SERVICE_DEFINE(fall_svc,
    BT_GATT_PRIMARY_SERVICE(&fall_service_uuid),

    /* NOTIFY → FALL hacia móvil */
    BT_GATT_CHARACTERISTIC(&fall_char_notify_uuid.uuid,
                           BT_GATT_CHRC_NOTIFY,
                           BT_GATT_PERM_NONE,
                           NULL,
                           NULL,
                           notify_buf),

    BT_GATT_CCC(ccc_cfg_changed,
                BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),

    /* WRITE → GPS desde móvil */
    BT_GATT_CHARACTERISTIC(&fall_char_write_uuid.uuid,
                           BT_GATT_CHRC_WRITE,
                           BT_GATT_PERM_WRITE,
                           NULL,
                           write_rx,
                           NULL)
);

/* ================= SEND ================= */

int ble_send(uint8_t *data, uint16_t len)
{
    if (!current_conn) {
        return -ENOTCONN;
    }

    if (!notify_enabled) {
        return -EACCES;
    }

    if (len > sizeof(notify_buf)) {
        return -EINVAL;
    }

    memcpy(notify_buf, data, len);

    return bt_gatt_notify(current_conn,
                          &fall_svc.attrs[1],
                          notify_buf,
                          len);
}

/* ================= TEST ================= */

static int ble_send_test(void)
{
    char msg[] = "FALL";

    int err = ble_send((uint8_t *)msg, sizeof(msg) - 1);

    printk("TEST FALL SEND: %d\n", err);

    return err;
}

void ble_test_fall(void)
{
    ble_send_test();
}

/* ================= CONNECTION CALLBACKS ================= */

static void connected(struct bt_conn *conn, uint8_t err)
{
    if (err) {
        printk("BLE connect error: %d\n", err);
        return;
    }

    current_conn = bt_conn_ref(conn);

    printk("BLE conectado\n");
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
    ARG_UNUSED(reason);

    if (current_conn) {
        bt_conn_unref(current_conn);
        current_conn = NULL;
    }

    notify_enabled = false;

    printk("BLE desconectado\n");

    start_advertising();
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = connected,
    .disconnected = disconnected,
};

/* ================= ADVERTISING ================= */

static void start_advertising(void)
{
    int err;

    err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1,
                          ad,
                          ARRAY_SIZE(ad),
                          sd,
                          ARRAY_SIZE(sd));

    if (err) {
        printk("Advertising error: %d\n", err);
    } else {
        printk("Advertising iniciado\n");
    }
}

/* ================= INIT ================= */

void ble_init(void)
{
    int err;

    err = bt_enable(NULL);
    if (err) {
        printk("bt_enable error: %d\n", err);
        return;
    }

    printk("Bluetooth inicializado\n");

    start_advertising();
}