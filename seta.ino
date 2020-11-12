#include <ArduinoJson.h>
#include <Ethernet.h>
#include <SPI.h>

const char *CODICE_FERMATA = "MO2076";
const char *FERMATA_OPPOSTA = "si";

#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 20, 4);

EthernetClient client;

char nomeFermata[32];
char oraAttuale[8];

// Connect to HTTP server
void connectToServer() {
    client.setTimeout(10000);
    if (!client.connect("setaapi2.bitrey.it", 80)) {
        Serial.println(F("Connection failed"));
        return;
    }

    //  Serial.println(F("Connected to server!"));
}

// Check HTTP status
void checkHttpStatus() {
    char status[32] = {0};
    client.readBytesUntil('\r', status, sizeof(status));
    // It should be "HTTP/1.0 200 OK" or "HTTP/1.1 200 OK"
    if (strcmp(status + 9, "200 OK") != 0) {
        Serial.print(F("Unexpected response: "));
        Serial.println(status);
        return;
    }
}

// Skip HTTP headers
void skipHttpHeaders() {
    char endOfHeaders[] = "\r\n\r\n";
    if (!client.find(endOfHeaders)) {
        Serial.println(F("Invalid response"));
        return;
    }
}

int cursoreFermata() {
    return strlen(nomeFermata) < 16 ? (16 - strlen(nomeFermata)) / 2 : 0;
}

bool hasStarted = false;

void setup() {
    // Initialize LCD Display
    lcd.init();
    lcd.backlight();

    lcd.setCursor(2, 0);
    lcd.print("SETA S.p.A.");
    lcd.setCursor(1, 1);
    lcd.print("Caricamento...");

    // Initialize Serial communications
    Serial.begin(9600);
    while (!Serial) continue;
    Serial.println("Comunicazione seriale stabilita");

    byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
    if (!Ethernet.begin(mac)) {
        Serial.println(F("Failed to configure Ethernet"));
        return;
    }

    connectToServer();

    client.print(F("GET /codice/"));
    client.print(CODICE_FERMATA);
    client.println(F(" HTTP/1.0"));
    client.println(F("Host: setaapi2.bitrey.it"));
    client.println(F("Connection: close"));
    if (client.println() == 0) {
        Serial.println(F("Failed to send request"));
        return;
    }

    checkHttpStatus();

    skipHttpHeaders();

    // Allocate the JSON document
    // Use arduinojson.org/v6/assistant to compute the capacity.
    const size_t capacity = JSON_OBJECT_SIZE(3) + 50;
    //  JSON_OBJECT_SIZE(2) + 40;

    // StaticJsonDocument<capacity> doc;
    DynamicJsonDocument doc(capacity);

    // Parse JSON object
    DeserializationError error = deserializeJson(doc, client);
    if (error) {
        Serial.print(F("setup deserializeJson() failed: "));
        Serial.println(error.c_str());
        return;
    }

    lcd.clear();

    strcpy(nomeFermata, doc["nome"]);
    strcpy(oraAttuale, doc["orario"]);

    lcd.setCursor(cursoreFermata(), 0);
    lcd.print(nomeFermata);
    lcd.setCursor(5, 1);
    lcd.print(oraAttuale);

    // Disconnect
    client.stop();

    delay(2000);

    hasStarted = true;
}

void loop() {
    if (!hasStarted) return;

    delay(1000);

    connectToServer();

    // delay(1000);

    client.print(F("GET /corse/mo/"));
    client.print(CODICE_FERMATA);
    client.print(F("?corseFermataOpposta="));
    client.print(FERMATA_OPPOSTA);
    client.println(F(" HTTP/1.0"));
    client.println(F("Host: setaapi2.bitrey.it"));
    client.println(F("Connection: close"));
    if (client.println() == 0) {
        Serial.println(F("Failed to send request"));
        return;
    }

    checkHttpStatus();

    skipHttpHeaders();

    // Allocate the JSON document
    // Use arduinojson.org/v6/assistant to compute the capacity.
    const size_t capacity = JSON_ARRAY_SIZE(5) + JSON_OBJECT_SIZE(1) +
                            4 * JSON_OBJECT_SIZE(9) + 300;
    // Serial.println("alloco");

    // StaticJsonDocument<capacity> doc;
    DynamicJsonDocument doc(capacity);

    // Serial.println("allocato");

    // Parse JSON object
    DeserializationError error = deserializeJson(doc, client);
    if (error) {
        Serial.print(F("loop deserializeJson() failed: "));
        Serial.println(error.c_str());
        return;
    }

    // Viene impostato a vero se qualche autobus viene stampato
    bool noBuses = true;

    lcd.clear();

    JsonArray arr = doc.as<JsonArray>();

    // Orario
    JsonObject ora = arr[0];
    if (ora.containsKey("orario")) {
        const char *orario = ora["orario"];
        strcpy(oraAttuale, orario);
        ora.clear();

        lcd.setCursor(cursoreFermata(), 0);
        lcd.print(nomeFermata);
        lcd.setCursor(5, 1);
        lcd.print(oraAttuale);

        delay(3000);
    }

    // Corse
    for (int i = 1; i < arr.size(); i++) {
        JsonObject bus = arr[i];

        if (!bus.containsKey("linea")) continue;

        const char *linea = bus["linea"];
        const char *destinazione = bus["destinazione"];
        const char *pianificato = bus["pianificato"];
        const char tipoLinea = bus["tipo_linea"];
        const int minArrivo = bus["min_all_arrivo_val"];
        const char *tempoReale = bus["temporeale"];

        // if (!linea || strlen(linea) == 0) { /* continue ; */
        //     //      DEBUG
        //     Serial.println(F("No linea"));
        // }

        noBuses = false;

        lcd.clear();

        lcd.setCursor(0, 0);
        lcd.print(i);
        lcd.print("/");
        lcd.print(arr.size() - 1);
        lcd.print(" ");

        lcd.print(linea);
        lcd.print(" ");

        lcd.print(destinazione);

        if (minArrivo != 9999) {
            lcd.setCursor(0, 1);
            lcd.print(pianificato);
            if (strcmp(pianificato, tempoReale)) {
                lcd.print("->");
                lcd.print(tempoReale);
                lcd.print(" ");
                lcd.print(minArrivo);
                lcd.print("min");
            } else {
                lcd.print(" tra ");
                lcd.print(minArrivo);
                lcd.print(" min");
            }
        } else {
            lcd.setCursor(0, 1);
            lcd.print(pianificato);
            lcd.print(" (pianific)");
        }

        delay(3000);
    }
    if (arr.size() == 0 || noBuses) {
        lcd.setCursor(0, 0);
        lcd.print(oraAttuale);
        if (strlen(nomeFermata) > 0) {
            lcd.setCursor(6, 0);
            lcd.print(nomeFermata);
        } else {
            lcd.setCursor(0, 0);
            lcd.print("SETA S.p.A.");
        }
        lcd.setCursor(0, 1);
        lcd.print("Nessuna corsa");
    }

    delay(1000);

    // Disconnect
    client.stop();

    doc.clear();
}
