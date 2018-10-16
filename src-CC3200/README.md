The code running on the TI CC3200-LAUNCHXL itself.
Project based on: getting_started_with_wlan_station (example furnished with CCS)
Functionality:
- Alphanumerical display on a 64x32 LED matrix ("usual" ASCII characters + Hungarian diacritics)
- Score counter for table tennis using two external buttons
- Internet connection over WLAN
  - No-IP hostname update so the device can be accessed by a remote host even if it has a dynamic IP addresswith
  - Remote configuration / readout features over TCP-IP
Note: WLAN_TCP.c, static long NoIPUpdate() - e-mail address, hostname and authentication base masked

Target platform: TI CC3200-LAUNCHXL - http://www.ti.com/tool/CC3200-LAUNCHXL
Compiler: TI Code Composer Studio v7 - http://www.ti.com/tool/CCSTUDIO
