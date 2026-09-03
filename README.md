# Totem ZMK config

GEIGEIGEIST [Totem](https://github.com/GEIGEIGEIST/totem) on Seeed XIAO nRF52840.

- Firmware: [zmkfirmware/zmk](https://github.com/zmkfirmware/zmk) (`xiao_ble//zmk`)
- Left half: ZMK Studio (`studio-rpc-usb-uart`)
- Pairing: `CONFIG_ZMK_BLE_PASSKEY_ENTRY` — type the six digits, then Enter. Unplug USB to type over Bluetooth.

Build via GitHub Actions. Flash `totem_left` / `totem_right` UF2s (double-tap the XIAO reset next to USB-C).
