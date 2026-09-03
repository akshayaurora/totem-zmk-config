/*
 * Pause host advertising after the selected profile has been disconnected
 * for CONFIG_TOTEM_ADV_THROTTLE_TIMEOUT_MIN. A keypress restarts advertising
 * directly; stock ZMK cannot resume after a bare bt_le_adv_stop().
 *
 * Central only. Does not patch zmkfirmware/zmk ble.c.
 */

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>

#include <zmk/ble.h>
#include <zmk/event_manager.h>
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/events/position_state_changed.h>

#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)

#define THROTTLE_MIN CONFIG_TOTEM_ADV_THROTTLE_TIMEOUT_MIN

static const struct bt_le_adv_param totem_adv_conn_param =
    BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONN | BT_LE_ADV_OPT_USE_NAME | BT_LE_ADV_OPT_FORCE_NAME_IN_AD,
                    BT_GAP_ADV_FAST_INT_MIN_2, BT_GAP_ADV_FAST_INT_MAX_2, NULL);

static const struct bt_data totem_adv_data[] = {
    BT_DATA_BYTES(BT_DATA_GAP_APPEARANCE, BT_BYTES_LIST_LE16(CONFIG_BT_DEVICE_APPEARANCE)),
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA_BYTES(BT_DATA_UUID16_SOME, BT_UUID_16_ENCODE(BT_UUID_HIDS_VAL),
                  BT_UUID_16_ENCODE(BT_UUID_BAS_VAL)),
};

static bool totem_adv_dark;

static int totem_adv_start_open(void) {
    int err = bt_le_adv_start(&totem_adv_conn_param, totem_adv_data, ARRAY_SIZE(totem_adv_data),
                              NULL, 0);

    if (err == -EALREADY) {
        return 0;
    }

    return err;
}

static void totem_adv_go_dark(struct k_work *work) {
    ARG_UNUSED(work);

    if (zmk_ble_active_profile_is_connected()) {
        totem_adv_dark = false;
        return;
    }

    bt_le_adv_stop();
    totem_adv_dark = true;
}

static K_WORK_DELAYABLE_DEFINE(totem_adv_dark_work, totem_adv_go_dark);

static void totem_adv_arm(void) {
    if (zmk_ble_active_profile_is_connected()) {
        totem_adv_dark = false;
        k_work_cancel_delayable(&totem_adv_dark_work);
        return;
    }

    k_work_reschedule(&totem_adv_dark_work, K_MINUTES(THROTTLE_MIN));
}

static void totem_adv_wake(void) {
    if (!totem_adv_dark || zmk_ble_active_profile_is_connected()) {
        return;
    }

    if (totem_adv_start_open() == 0) {
        totem_adv_dark = false;
        totem_adv_arm();
    }
}

static int totem_adv_profile_listener(const zmk_event_t *eh) {
    ARG_UNUSED(eh);

    if (zmk_ble_active_profile_is_connected()) {
        totem_adv_dark = false;
    }

    totem_adv_arm();
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(totem_adv_throttle_profile, totem_adv_profile_listener);
ZMK_SUBSCRIPTION(totem_adv_throttle_profile, zmk_ble_active_profile_changed);

static int totem_adv_key_listener(const zmk_event_t *eh) {
    const struct zmk_position_state_changed *ev = as_zmk_position_state_changed(eh);

    if (ev == NULL || !ev->state) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    totem_adv_wake();
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(totem_adv_throttle_key, totem_adv_key_listener);
ZMK_SUBSCRIPTION(totem_adv_throttle_key, zmk_position_state_changed);

static int totem_adv_throttle_init(void) {
    totem_adv_arm();
    return 0;
}

SYS_INIT(totem_adv_throttle_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

#endif /* CONFIG_ZMK_SPLIT_ROLE_CENTRAL */
