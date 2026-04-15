# ESP32 Wi-Fi Airdoom

A simple ESP32 pen-testing firmware for ESP32 boards.

> ⚠️ Use only on your own network or with explicit permission.

---

## Quick Start

- Flash the prebuilt firmware with `esptool.py` (Linux/macOS) or the Espressif Flash Download Tool (Windows).
- Connect your phone/laptop to WiFi `Airdoom` and open http://192.168.4.1 to use the UI.

---

## Demo

Demo video: [Demo.mp4](Demo.mp4)


## Hardware Requirements

- ESP32 board (WROOM, WROVER, DevKit, or similar)
- Minimum 4MB flash
- USB cable for flashing


Esp32: [esp32.png](esp32.png)


---

## Flash Instructions

### Linux / macOS using esptool

```bash
# Flash the ESP32 directly from the firmware folder
esptool.py -p /dev/ttyS5 -b 115200 --after hard_reset write_flash \
  --flash_mode dio --flash_freq 40m --flash_size detect \
  0x8000 firmware/partition-table.bin \
  0x1000 firmware/bootloader.bin \
  0x10000 firmware/airdoom.bin
```


### Notes

- `firmware/partition-table.bin` = partition table
- `firmware/bootloader.bin` = bootloader image
- `firmware/airdoom.bin` = main firmware

-- Replace `/dev/ttyS5` with your serial port. Common examples:
  - Linux: `/dev/ttyUSB0` or `/dev/ttyUSB1`
  - macOS: `/dev/tty.SOMETHING` (use `ls /dev/tty.*` to list)
  - Windows: `COM3` (use Device Manager to find the COM port)

  To find the correct port on Linux run:
  ```bash
  dmesg | grep -i tty
  ls /dev/ttyUSB*
  ```

  On Windows, check Device Manager → Ports (COM & LPT) for the COM number.

### Tools

- Linux/macOS: https://github.com/espressif/esptool
- Windows: https://www.espressif.com/en/support/download/other-tools

---

## Connect to the Device

1. Open WiFi settings.
2. Connect to network `Airdoom`.
3. Password: `airdoomadmin`
4. Open browser and go to `http://192.168.4.1`
5. The Airdoom UI should load.

---

## Attack Types

- Passive Sniffer: silently capture raw Wi-Fi packets into a `.pcap` file.
- Handshake Capture: force reconnect or wait for a client to capture WPA/WPA2 handshake.
- PMKID Attack: retrieve PMKID directly from the AP without needing an active client.
- Deauth DoS: send deauthentication frames to disconnect clients from the target AP.
- Beacon Spam: flood fake SSIDs to overwhelm nearby Wi-Fi scans.
- Mass Deauther: jam all nearby networks across channels for broad disruption.

Use the device UI to select the attack type and start the target scan.

---

## Legal

Use this firmware only for educational or authorized testing.
