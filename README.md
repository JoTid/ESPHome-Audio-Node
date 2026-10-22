# ESPHome Audio Node

ESPHome-Audioplayer auf einem ESP32-S3 CAM Development Board mit N16R8-Modul, externem PCM5102A-I2S-DAC und Anbindung an Home Assistant. Die aktuelle Konfiguration ist auf Sprachansagen in Mono ausgelegt.

## Hardware und Verkabelung

Die Pins beziehen sich auf die Konfiguration in [`esphome-audio-node.yaml`](./esphome-audio-node.yaml):

Das verwendete Board-Pinout ist in [docs/esp32-s3_cam_pinout.png](./docs/esp32-s3_cam_pinout.png) abgebildet.

| ESP32-S3 CAM Development Board | PCM5102A / ICS-43434 |
| --- | --- |
| GPIO 1 | PCM5102A LRCK / LCK / WS; ICS-43434 WS |
| GPIO 2 | PCM5102A BCLK / BCK; ICS-43434 SCK |
| GPIO 14 | PCM5102A DIN |
| GPIO 21 | ICS-43434 SD / DOUT |
| GPIO 43 | Ausgang des Bewegungsmelders |
| GND | GND der Module und des Bewegungsmelders |

Die Kameraanschlüsse belegen unter anderem GPIOs 4–13 und 15–18; diese werden daher nicht für Audio verwendet.

Das Board hat einen Onboard-microSD-Anschluss. Laut Pinout sind dessen Signale:

| ESP32-S3 GPIO | Onboard-microSD |
| --- | --- |
| GPIO 39 | SD_CLK |
| GPIO 38 | SD_CMD |
| GPIO 40 | SD_DATA / DAT0 |

Die SD-Karte wird im 1-Bit-SDMMC-Modus eingebunden. Lege die WAV-Dateien direkt in das Stammverzeichnis der Karte. Bei jeder neuen Bewegung startet das Gerät zufällig eine passende Datei; die zuletzt abgespielte Datei wird nach Möglichkeit nicht direkt wiederholt. Die Karte muss FAT32-formatiert sein. ESPHome formatiert sie bei einem Fehler ausdrücklich nicht.

Unterstützt werden PCM-WAV-Dateien mit 16 kHz, 16 Bit und einem Kanal (Mono). Andere Abtastraten, Bitbreiten oder komprimierte WAV-Dateien werden übersprungen. Die Wiedergabe wird direkt von der Karte zum I²S-Lautsprecher gestreamt, ohne die vollständige Datei in den RAM zu laden. Wenn die Karte nicht eingebunden werden kann oder keine passende Datei enthält, steht der Grund im ESPHome-Log.

Die Bewegungsmeldung wird weiterhin an Home Assistant übertragen. Da die Wiedergabe nun lokal startet, deaktiviere eine vorhandene Home-Assistant-Automation, die bei derselben Bewegung zusätzlich Audio auf diesem Gerät startet, um doppelte oder überlagerte Wiedergabe zu vermeiden.

Verbinde den L/R-Pin des ICS-43434 mit GND, damit das Mikrofon den linken I²S-Kanal ausgibt. Versorge beide Module entsprechend deren Angaben. Verbinde den Audioausgang des DAC mit einem geeigneten Verstärker oder Aktivlautsprecher.

Der AM312 wird mit seinem digitalen Ausgang an GPIO 43 angeschlossen. Die Konfiguration erwartet HIGH bei erkannter Bewegung und verwendet keinen internen Pull-up oder Pull-down, damit der Eingang den Sensorausgang nicht unnötig belastet. Sensor und ESP32 müssen eine gemeinsame Masse (GND) haben. Achte darauf, dass am ESP32-Eingang höchstens 3,3 V anliegen; verbinde keinen 5-V-Ausgang direkt mit GPIO 43. In Home Assistant erscheint der Eingang als Binärsensor mit der Geräteklasse „Bewegung“ und kann in Automationen verwendet werden.

Die serielle UART-Ausgabe des ESPHome-Loggers ist deaktiviert (`baud_rate: 0`), weil GPIO 43 als Eingang für den Bewegungsmelder verwendet wird. Die Warnmeldungen bleiben über die ESPHome-/Home-Assistant-Verbindung verfügbar.

## Audio-Konfiguration

- I2S-Ausgabe über `speaker.i2s_audio` mit externem DAC
- Mono, 16 kHz und 16 Bit
- WAV für die Home-Assistant-Announcement-Pipeline
- Zufällige lokale PCM-WAV-Wiedergabe von der microSD bei Bewegung
- ICS-43434 als I²S-Mikrofon, 16 kHz, linker Kanal
- Home Assistant Voice Assistant mit Wake-Word-Unterstützung
- `logger` ist auf `WARN` gesetzt
- UART-Logging ist deaktiviert, da GPIO 43 als Sensoreingang verwendet wird
- CPU-Takt ist auf 240 MHz gesetzt
- ESP32-S3-N16R8 mit 16 MB Flash und Octal-PSRAM
- Media-Player-Puffer ist auf 100 kB begrenzt, um RAM zu sparen

Falls dein ESP32 bei 240 MHz instabil läuft oder neu startet, ändere `cpu_frequency` in der YAML-Datei auf `160MHz`.

## Voraussetzungen

- Python 3.12
- [Poetry](https://python-poetry.org/docs/#installing-with-the-official-installer)
- ESPHome 2026.9.1 oder neuer (wird durch Poetry installiert)
- ESP32-S3 CAM Development Board mit N16R8-Modul und Onboard-microSD-Anschluss
- Home Assistant mit ESPHome-Integration

## Secrets einrichten

Lege für den lokalen Build eine `secrets.yaml` im Projektverzeichnis an. Ersetze die Beispielwerte durch deine eigenen Daten. Den API-Verschlüsselungsschlüssel kannst du in ESPHome erzeugen lassen.

```yaml
wifi_ssid: "Dein WLAN"
wifi_password: "Dein WLAN-Passwort"

esphome_audio_node_ha_api_key: "DEIN_GENERIERTER_API_VERSCHLUESSELUNGSSCHLUESSEL"
esphome_audio_node_ota_password: "Dein-OTA-Passwort"
esphome_audio_node_ap_password: "Dein-Fallback-AP-Passwort"

esphome_audio_node_static_ip: "192.168.1.50"
esphome_audio_node_gateway: "192.168.1.1"
esphome_audio_node_subnet: "255.255.255.0"
```

Wähle eine freie statische IP-Adresse aus deinem Netzwerk. Passe Gateway und Subnetz bei Bedarf an. Die Datei `secrets.yaml` enthält private Zugangsdaten und sollte nicht eingecheckt oder veröffentlicht werden.

Das OTA-Passwort bleibt zunächst für das Update vorhandener Firmware erhalten. ESPHome 2026.9.1 kann zusätzlich verschlüsseltes OTA anbieten. Nach dem ersten Update muss das Gerät im Log `Encryption: offered, plaintext accepted` melden; erst dann sollte das OTA-Passwort in einem zweiten Update durch `encryption: {}` ersetzt werden. Dabei wird derselbe Schlüssel wie für die Native API verwendet.

## Installieren und bauen

```powershell
poetry env use 3.12
poetry install
poetry run esphome config esphome-audio-node.yaml
poetry run esphome compile esphome-audio-node.yaml
```

Zum Flashen oder Hochladen der Firmware:

```powershell
poetry run esphome run esphome-audio-node.yaml
```

Alternativ kannst du die YAML-Konfiguration in ESPHome Device Builder einbinden und dort bauen sowie flashen. Stelle sicher, dass die referenzierten Secrets auch dort eingerichtet sind.
