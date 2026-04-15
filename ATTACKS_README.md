# 📡 ESP32 Wi-Fi Penetration Tool: Attack Modes


Here is the technical description of all the available penetration attacks your ESP32 Wi-Fi Tool brings to the table:

### 1. Passive Sniffer
* **How it works:** The ESP32 switches into generic promiscuous mode and acts as a silent packet sniffer. It intercepts and captures all standard IEEE 802.11 Management, Control, and Data frames flowing through the air on the targeted channel.
* **Use Case:** To passively capture completely raw network traffic into a `.pcap` file format. This file can later be downloaded and imported into packet analyzers like Wireshark without triggering any alarms or alerts (completely stealthy).

### 2. Handshake Capture
* **How it works:** To crack WPA/WPA2 Personal networks, a valid 4-way EAPOL key handshake is needed. This attack kicks out connected stations using various methods so they try to reconnect, generating the desired 4-way handshake, which is then serialized directly into a Hashcat-compatible `.hccapx` format.
* **Methods:**
  * **Rogue AP:** Creates an evil twin of the target AP by cloning its BSSID and SSID which forces clients to mistakenly associate, triggering the handshake frames.
  * **Broadcast Deauth:** Injects forged `deauthentication` frames on behalf of the AP to brutally force clients offline. When they reconnect, the frames are caught.
  * **Passive Only:** Waits completely silently on the channel until a roaming generic device associates itself entirely organically. 

### 3. PMKID Attack
* **How it works:** Also known as a "Clientless WPA/WPA2 Cracking Attack", this attack does not require any active clients to be connected to the router. The ESP32 communicates sequentially with the Access Point directly by initializing an EAPOL authentication and retrieves the PMKID hash which can be brute-forced offline later.

### 4. Deauth DoS (Wi-Fi Jammer)
* **How it works:** A targeted Denial of Service (DoS) attack designed to entirely paralyze a specific Wi-Fi network preventing any endpoints from sustaining an active internet connection.
* **Methods:**
  * **Rogue AP:** Spams authentication confusion using an identical AP.
  * **Broadcast Deauth:** Fires forged `deauthentication` packets every 100 milliseconds directly at the target AP disrupting the connection line instantly.
  * **Combined (Ultimate Jammer):** Deploys both techniques identically. Connected endpoints are aggressively kicked out while a Rogue AP instantly traps them as they try connecting back.

### 5. Beacon Spam (SSID Flooding)
* **How it works:** The ESP32 floods the 2.4GHz spectrum with hundreds of fake IEEE-802.11 Beacon management frames every second. Each frame corresponds to a randomized fake MAC address and a fake SSID name (like "Hacked_By_Me").
* **Use Case:** Causes extreme clutter and potential application crashing in nearby hardware's Wi-Fi scanners simply due to handling such massive unexpected quantities of available networks simultaneously.

### 6. Mass Deauther (Nuke Mode) 💥
* **How it works:** This is an extreme widespread jamming event. Instead of sticking sequentially to a single selected Access Point, the radio aggressively jumps through all available Wi-Fi channels (1 through 13). At every hop, it loops through the cached AP records stored from the general scan, finding endpoints natively matching the operating channel, and unleashes 3 precise deauthentication frame bursts.
* **Use Case:** Instantly drops wireless connectivity for every susceptible IEEE 802.11 device within physical range recursively, irrespective of which respective Access Point it is assigned to. Causes immediate total connection blackout.

---

*Disclaimer: This tool should solely be utilized for educational security research regarding one's own independent network. Unauthorized deployment equates directly to malicious interference of communications structure.*
