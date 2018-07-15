# Wordclock
en: An ESP8266 (Wemos D1 Mini) based wordclock using WS2812B LEDs
de: Eine ESP8266 (Wemos D1 Mini) basierte Word Clock mit WS2812B LEDs

# english

## idea
The basic idea of a word clock is to show the time as text consisting of lit elements in a grid of letters.

## drawbacks
The text interpretation of the current time is heavily language dependant. New languages don't mean a simple change of skin (letter grid) but typically code changes too.
Currently this project is focused on a german word clock - but feel free to add english or other languages.

# Deutsch

## Idee
Eine Wordclock ist eine Uhr, die die Zeit als Text anzeigt. Dieser ergibt sich auch leuchtenden Elementen einer Buchstaben-Matrix. Der hier vorliegende Code ermöglicht den Betrieb einer solchen Uhr mittels eines ESP8266-basierten Boards sowie einer Reihe von WS2812B ("Neo-Pixel") LEDs für die einzelnen Buchstaben.

## Arduino Setup
Der Code wurde mit einer Arduino 1.8.5 Installation erfolgreich übersetzt und läuft auf einem Wemos D1 Mini Klon.

## Arduino Libraries
Der Code verwendet insbesondere die folgenden Arduino-Bibliotheken:
- ArduinoJson (by Benoit Blanchon) - v5.2.0 getestet
- ArduinoOTA (BuiltIn)
- FastLED (by Daniel Garcia) - v3.1.6 getestet
- Timezone (by Jack Christensen) - v1.2.0 getestet
- WifiManager (by tzapu) - v0.12.0 getestet

## Hardware Setup
Herz des ganzen Setups ist ein Wemos D1 Mini Klon. An seinem D2-Anschluss ist eine Kette von 110 WS2812B LEDs (11x 10 LEDs, aus einem 5m 30 LED/m Streifen geschnitten) angeschlossen. Diese sind schlangenförmig von oben rechts nach unten verbunden. Der Anschluss der ersten Zeile erfolgt also vo aufn rechts, die zweite Zeile ist dann links, die dritte wieder rechts angeschlossen, usw. Zusätzlich sind an D5 und D7 Taster angeschlossen die in der Software "frei" belegt werden können.

Als Gehäuse kam ein Ikea Bilderrahmen 50x50 (dicker Rand mit 3D-Effekt innen) zum Einsatz. Als Diffusor wurde eine Polystyrolplatte aus dem Baumarkt (2,5mm dick) besorgt und mit dem Cutter zugeschnitten. Auf den Diffusor wurde schwarze Klebefolie aufgeklebt und die Maske der Buchstaben mit einem Cutter ausgeschnitten (sehr zeitaufwändig!). Dieser Schritt ist mit einem passenden Lasercutter im Zugriff sicher deutlich leichter zu haben und ggf. auch schicker.

Wemos, die Taster und ein Schalter der Stromzufuhr für die LEDs (falls eine falsche Netzteilstärke ausgewählt wurde) sind auf einer kleinen Platine zusammengebraten und an der Rückwand des Rahmens befestigt, auf der auch die LED-Streifen kleben.

Zusätzlich muss ein Separator für die einzelnen Buchstaben erstellt werden. Bei mir waren es passend breite Streifen von Tonkarton, an den passenden Stellen halb eingeschnitten und damit zu einem großen Raster zusammengesteckt. Leider ist meine "billige" Variante nicht ausreichend dicht, so dass es leichte Überblendungen eines Buchstaben auf einen benachbarten aber unbeleuchteten Buchstaben gibt.

Entsprechende Darstellungen werde ich noch nachliefern.
