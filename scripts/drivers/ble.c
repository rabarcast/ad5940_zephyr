#include "ble.h"

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/conn.h>
#include <string.h>

static struct bt_conn *current_conn = NULL;
static bool notify_enabled = false;

/* UUIDs personalizados */

#define BT_UUID_FALL_SERVICE_VAL \
    BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef0)

#define BT_UUID_FALL_CHAR_VAL \
    BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef1)

static struct bt_uuid_128 fall_service_uuid =
    BT_UUID_INIT_128(BT_UUID_FALL_SERVICE_VAL);

static struct bt_uuid_128 fall_char_uuid =
    BT_UUID_INIT_128(BT_UUID_FALL_CHAR_VAL);

/* buffer notify */
static uint8_t notify_buf[32];

/* CCC */

static void ccc_cfg_changed(const struct bt_gatt_attr *attr,
                            uint16_t value)
{
    ARG_UNUSED(attr);

    notify_enabled = (value == BT_GATT_CCC_NOTIFY);
}

/* Servicio BLE */

BT_GATT_SERVICE_DEFINE(fall_svc,
    BT_GATT_PRIMARY_SERVICE(&fall_service_uuid),

    BT_GATT_CHARACTERISTIC(&fall_char_uuid.uuid,
                           BT_GATT_CHRC_NOTIFY,
                           BT_GATT_PERM_NONE,
                           NULL,
                           NULL,
                           notify_buf),

    BT_GATT_CCC(ccc_cfg_changed,
                BT_GATT_PERM_READ | BT_GATT_PERM_WRITE)
);

/* callbacks conexión */

static void connected(struct bt_conn *conn, uint8_t err)
{
    if (err == 0) {

        current_conn = bt_conn_ref(conn);

        printk("BLE conectado\n");
    }
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
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = connected,
    .disconnected = disconnected,
};

/* advertising */

static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS,
                  (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
};

static const struct bt_data sd[] = {
    BT_DATA(BT_DATA_NAME_COMPLETE,
            "FALL_SENSOR",
            11),
};

void ble_init(void)
{
    int err;

    err = bt_enable(NULL);

    if (err) {

        printk("Error bt_enable: %d\n", err);

        return;
    }

    printk("Bluetooth inicializado\n");

    err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1,
                          ad,
                          ARRAY_SIZE(ad),
                          sd,
                          ARRAY_SIZE(sd));

    if (err) {

        printk("Error advertising: %d\n", err);

        return;
    }

    printk("Advertising iniciado\n");
}

/* notify */

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