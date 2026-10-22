# ESPHome Audio Node

ESPHome-Audioplayer auf einem ESP32-S3-WROOM-1-N16R8 im ESP32-S3-DevKitC-1-Format mit externem PCM5102A-I2S-DAC und Anbindung an Home Assistant. Die Konfiguration spielt bei Bewegung zufällig eingebettete Audiodateien ab.

## Hardware und Verkabelung

Die Pins beziehen sich auf die Konfiguration in [`esphome-audio-node.yaml`](./esphome-audio-node.yaml):

| ESP32-S3-DevKitC-1 | PCM5102A / ICS-43434 |
| --- | --- |
| GPIO 4 | PCM5102A LRCK / LCK / WS; ICS-43434 WS |
| GPIO 5 | PCM5102A BCLK / BCK; ICS-43434 SCK |
| GPIO 6 | PCM5102A DIN |
| GPIO 7 | ICS-43434 SD / DOUT |
| GPIO 8 | WS2812B-Datenleitung (bis zu 50 LEDs) |
| GPIO 21 | Ausgang des Bewegungsmelders (AM312) |
| GND | GND beider Module |

Das Pinout für ESP32-S3-DevKitC-1 ist unter [`docs/ESP32-S3-DevKitC-1_pinout.png`](./docs/ESP32-S3-DevKitC-1_pinout.png) abgelegt. GPIO 4 bis GPIO 8 sowie GPIO 21 sind auf der abgebildeten Platine herausgeführt. Die GPIOs 19/20 (USB) und 35–37 (vom Octal-PSRAM belegt) werden nicht verwendet.

Verbinde den L/R-Pin des ICS-43434 mit GND, damit das Mikrofon den linken I²S-Kanal ausgibt. PCM5102A und Mikrofon teilen sich LRCK und BCLK; ihre Datenleitungen sind separat. Versorge beide Module entsprechend deren Angaben. Verbinde den Audioausgang des DAC mit einem geeigneten Verstärker oder Aktivlautsprecher.

Der AM312 wird mit seinem digitalen Ausgang an GPIO 21 angeschlossen. Die Konfiguration erwartet HIGH bei erkannter Bewegung und verwendet keinen internen Pull-up oder Pull-down, damit der Eingang den Sensorausgang nicht unnötig belastet. Sensor und ESP32 müssen eine gemeinsame Masse (GND) haben. Achte darauf, dass am ESP32-Eingang höchstens 3,3 V anliegen; verbinde keinen 5-V-Ausgang direkt mit GPIO 21. In Home Assistant erscheint der Eingang als Binärsensor mit der Geräteklasse „Bewegung“ und kann in Automationen verwendet werden.

Bis zu 50 WS2812B-LEDs werden in Reihe angeschlossen: GPIO 8 über einen Datenwiderstand (z. B. 330 Ω) an DIN der ersten LED, deren DOUT jeweils mit DIN der nächsten verbunden wird. Versorge den LED-Streifen mit einer für die Anzahl der LEDs ausreichend dimensionierten 5-V-Stromversorgung und verbinde deren GND mit GND des ESP32. Speise die LEDs nicht aus einem ESP32-GPIO. Für eine zuverlässige Datenübertragung von 3,3 V auf 5 V ist ein Logikpegelwandler empfehlenswert. Während der Wiedergabe beziehungsweise einer Audio-Ansage blinken die LEDs mit der in Home Assistant gewählten Farbe und Blinkdauer; bei Pause oder Ende der Wiedergabe gehen sie aus.

In Home Assistant stehen die Schalter „Halloween-Musik“ und „Halloween-LED“ zur Verfügung. „Halloween-Musik“ aktiviert die zufällige Sound-Wiedergabe bei Bewegung; beim Ausschalten wird eine laufende Wiedergabe gestoppt. „Halloween-LED“ aktiviert beziehungsweise deaktiviert das Blinken unabhängig von der Musik.

Farbe, Blink-AN-Zeit, Blink-AUS-Zeit und die Anzahl der blinkenden LEDs lassen sich in Home Assistant einstellen. Die Farbe kann aus Rot, Orange, Gelb, Grün, Türkis, Blau, Violett und Weiß ausgewählt werden. Die beiden Zeiten sind in Millisekunden einstellbar (jeweils 20 bis 5000 ms, in 10-ms-Schritten); voreingestellt sind 50 ms AN und 20 ms AUS. Die LED-Anzahl ist über ein Eingabefeld von 1 bis 50 einstellbar. Der LED-Streifen muss mindestens so viele angeschlossene Pixel haben wie ausgewählt sind.

## Audio-Konfiguration

- I2S-Ausgabe über `speaker.i2s_audio` mit externem DAC
- Mono, 16 kHz und 16 Bit
- MP3 für die Home-Assistant-Announcement-Pipeline
- Zehn MP3-Dateien werden beim Kompilieren von Home Assistant heruntergeladen, in die Firmware eingebettet und bei Bewegung zufällig abgespielt
- Bis zu 50 WS2812B-LEDs an GPIO 8 blinken während der Audio-Wiedergabe mit konfigurierbarer Anzahl, Farbe und Blinkdauer
- Home-Assistant-Schalter zum Aktivieren/Deaktivieren der bewegungsgesteuerten Sounds und des LED-Blinkens
- ICS-43434 als I²S-Mikrofon, 16 kHz, linker Kanal
- Home Assistant Voice Assistant mit Wake-Word-Unterstützung
- `logger` ist auf `WARN` gesetzt
- CPU-Takt ist auf 240 MHz gesetzt
- ESP32-S3-WROOM-1-N16R8 mit 16 MB Flash und 8 MB Octal-PSRAM
- Media-Player-Puffer ist auf 8 kB begrenzt

Falls dein ESP32 bei 240 MHz instabil läuft oder neu startet, ändere `cpu_frequency` in der YAML-Datei auf `160MHz`. Die eingebetteten Audiodateien müssen zusammen in die verfügbare Flash-Partition passen. Bei einem Build-Fehler wegen zu großer App-Datei müssen die Audio-Dateien verkleinert oder weniger Dateien eingebettet werden.

## Voraussetzungen

- Python 3.12
- [Poetry](https://python-poetry.org/docs/#installing-with-the-official-installer)
- ESPHome 2026.9.1 oder neuer (wird durch Poetry installiert)
- ESP32-S3-DevKitC-1 mit ESP32-S3-WROOM-1-N16R8
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

Beim Umstieg vom bisherigen ESP32 auf den ESP32-S3 muss die erste Installation per USB-Seriell erfolgen, nicht per OTA. Wähle dafür den COM-Port des DevKitC-1, zum Beispiel:

```powershell
poetry run esphome run esphome-audio-node.yaml --device COM4
```

Alternativ kannst du die YAML-Konfiguration in ESPHome Device Builder einbinden und dort bauen sowie flashen. Stelle sicher, dass die referenzierten Secrets auch dort eingerichtet sind.
