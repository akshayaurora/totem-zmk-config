/*
 * Pause host advertising after the selected profile has been disconnected
 * for CONFIG_TOTEM_ADV_THROTTLE_TIMEOUT_MIN. A keypress selects the active
 * profile again so stock ZMK restarts advertising.
 *
 * Central only. Does not patch zmkfirmware/zmk ble.c.
 */

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/kernel.h>

#include <zmk/ble.h>
#include <zmk/event_manager.h>
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/events/position_state_changed.h>

#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)

#define THROTTLE_MIN CONFIG_TOTEM_ADV_THROTTLE_TIMEOUT_MIN

static void totem_adv_go_dark(struct k_work *work) {
    ARG_UNUSED(work);

    if (zmk_ble_active_profile_is_connected()) {
        return;
    }

    bt_le_adv_stop();
}

static K_WORK_DELAYABLE_DEFINE(totem_adv_dark_work, totem_adv_go_dark);

static void totem_adv_arm(void) {
    if (zmk_ble_active_profile_is_connected()) {
        k_work_cancel_delayable(&totem_adv_dark_work);
        return;
    }

    k_work_reschedule(&totem_adv_dark_work, K_MINUTES(THROTTLE_MIN));
}

static int totem_adv_profile_listener(const zmk_event_t *eh) {
    ARG_UNUSED(eh);
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
    if (zmk_ble_active_profile_is_connected()) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    zmk_ble_prof_select((uint8_t)zmk_ble_active_profile_index());
    totem_adv_arm();
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
