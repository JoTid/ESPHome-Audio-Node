# ESPHome Audio Node

ESPHome-Audioplayer auf einem ESP32-D0WD-V3 mit externem PCM5102A-I2S-DAC und Anbindung an Home Assistant. Die Konfiguration spielt bei Bewegung zufällig eingebettete Audiodateien ab.

## Hardware und Verkabelung

Die Pins beziehen sich auf die Konfiguration in [`esphome-audio-node.yaml`](./esphome-audio-node.yaml):

| ESP32 | PCM5102A / ICS-43434 |
| --- | --- |
| GPIO 25 | PCM5102A LRCK / LCK / WS; ICS-43434 WS |
| GPIO 26 | PCM5102A BCLK / BCK; ICS-43434 SCK |
| GPIO 27 | PCM5102A DIN |
| GPIO 33 | ICS-43434 SD / DOUT |
| GPIO 22 | Ausgang des Bewegungsmelders |
| GND | GND beider Module |

Verbinde den L/R-Pin des ICS-43434 mit GND, damit das Mikrofon den linken I²S-Kanal ausgibt. Versorge beide Module entsprechend deren Angaben. Verbinde den Audioausgang des DAC mit einem geeigneten Verstärker oder Aktivlautsprecher.

Der AM312 wird mit seinem digitalen Ausgang an GPIO 22 angeschlossen. Die Konfiguration erwartet HIGH bei erkannter Bewegung und verwendet keinen internen Pull-up oder Pull-down, damit der Eingang den Sensorausgang nicht unnötig belastet. Sensor und ESP32 müssen eine gemeinsame Masse (GND) haben. Achte darauf, dass am ESP32-Eingang höchstens 3,3 V anliegen; verbinde keinen 5-V-Ausgang direkt mit GPIO 22. In Home Assistant erscheint der Eingang als Binärsensor mit der Geräteklasse „Bewegung“ und kann in Automationen verwendet werden.

## Audio-Konfiguration

- I2S-Ausgabe über `speaker.i2s_audio` mit externem DAC
- Mono, 16 kHz und 16 Bit
- MP3 für die Home-Assistant-Announcement-Pipeline
- Zwei MP3-Dateien werden beim Kompilieren von Home Assistant heruntergeladen, in die Firmware eingebettet und bei Bewegung zufällig abgespielt
- ICS-43434 als I²S-Mikrofon, 16 kHz, linker Kanal
- Home Assistant Voice Assistant mit Wake-Word-Unterstützung
- `logger` ist auf `WARN` gesetzt
- CPU-Takt ist auf 240 MHz gesetzt
- ESP32-D0WD-V3 mit Mindest-Chiprevision 3.1
- Media-Player-Puffer ist auf 8 kB begrenzt

Falls dein ESP32 bei 240 MHz instabil läuft oder neu startet, ändere `cpu_frequency` in der YAML-Datei auf `160MHz`. Die eingebetteten Audiodateien müssen zusammen in die verfügbare Flash-Partition passen. Bei einem Build-Fehler wegen zu großer App-Datei müssen die Audio-Dateien verkleinert oder weniger Dateien eingebettet werden.

## Voraussetzungen

- Python 3.12
- [Poetry](https://python-poetry.org/docs/#installing-with-the-official-installer)
- ESPHome 2026.9.1 oder neuer (wird durch Poetry installiert)
- ESP32-Board mit ESP32-D0WD-V3
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
poetry run esphome clean esphome-audio-node.yaml
poetry run esphome config esphome-audio-node.yaml
poetry run esphome compile esphome-audio-node.yaml
```

Zum Flashen oder Hochladen der Firmware:

```powershell
poetry run esphome run esphome-audio-node.yaml
```

Alternativ kannst du die YAML-Konfiguration in ESPHome Device Builder einbinden und dort bauen sowie flashen. Stelle sicher, dass die referenzierten Secrets auch dort eingerichtet sind.
