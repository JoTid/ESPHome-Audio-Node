# ESPHome Audio Node

ESPHome-Audioplayer auf einem ESP32 mit einem externen PCM5102A-I2S-DAC und Anbindung an Home Assistant. Die aktuelle Konfiguration ist auf Sprachansagen in Mono ausgelegt.

## Hardware und Verkabelung

Die Pins beziehen sich auf die Konfiguration in [`esphome-audio-node.yaml`](./esphome-audio-node.yaml):

| ESP32 | PCM5102A |
| --- | --- |
| GPIO 25 | LRCK / LCK / WS |
| GPIO 26 | BCLK / BCK |
| GPIO 22 | DIN |
| GND | GND |

Versorge das DAC-Modul entsprechend den Angaben des verwendeten Moduls. Verbinde außerdem den Audioausgang des DAC mit einem geeigneten Verstärker oder Aktivlautsprecher.

## Audio-Konfiguration

- I2S-Ausgabe über `speaker.i2s_audio` mit externem DAC
- Mono, 16 kHz und 16 Bit
- FLAC für die Home-Assistant-Announcement-Pipeline
- `logger` ist auf `WARN` gesetzt
- CPU-Takt ist auf 240 MHz gesetzt
- Mindest-Chiprevision ist 3.1 (ESP32-D0WD-V3)
- SRAM1 wird als IRAM verwendet (`sram1_as_iram`)
- Media-Player-Puffer ist auf 100 kB begrenzt, um RAM zu sparen

FLAC ist für diese Konfiguration eine geeignete Wahl: ESPHome benötigt dafür vergleichsweise wenig CPU. OPUS wäre keine passende Alternative bei 16 kHz, da der Codec eine Sample-Rate von 48 kHz voraussetzt.

Falls dein ESP32 bei 240 MHz instabil läuft oder neu startet, ändere `cpu_frequency` in der YAML-Datei auf `160MHz`.

## Voraussetzungen

- Python 3.12
- [Poetry](https://python-poetry.org/docs/#installing-with-the-official-installer)
- ESPHome 2026.9.1 oder neuer (wird durch Poetry installiert)
- Ein ESP32-Board, das mit `esp32dev` kompatibel ist
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
