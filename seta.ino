#include <ArduinoJson.h>
#include <Ethernet.h>
#include <SPI.h>

//#define CODICE_FERMATA "MO2076"
//#define FERMATA_OPPOSTA "si"

#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 20, 4);
// set the LCD address to 0x27 for a 16 chars and 2 line display

// const char* CODICE_FERMATA = "MO2076";
// const char* corseFermataOpposta = "si"

EthernetClient client;

char nomeFermata[32];
char oraAttuale[8];

// Initialize LCD screen
void initializeLCD() {
    // Initialize LCD Display
    lcd.init();
    lcd.backlight();

    lcd.setCursor(2, 0);
    lcd.print("SETA S.p.A.");
    lcd.setCursor(1, 1);
    lcd.print("Caricamento...");
}

// Initialize Serial communications
void initializeSerial() {
    Serial.begin(9600);
    while (!Serial) continue;
    Serial.println(F("Comunicazione seriale stabilita"));
}

// Initialize internet connection via ethernet
void initializeEthernet() {
    byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
    if (!Ethernet.begin(mac)) {
        Serial.println(F("Failed to configure Ethernet"));
        return;
    }
}

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

// Make request
void makeRequest(char *url) {
    //  USE URL!!!
    client.println(F("GET /codice/MO2076 HTTP/1.0"));
    client.println(F("Host: setaapi2.bitrey.it"));
    client.println(F("Connection: close"));
    if (client.println() == 0) {
        Serial.println(F("Failed to send request"));
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

void setup() {
    initializeLCD();

    initializeSerial();

    initializeEthernet();

    // Connect to HTTP server
    connectToServer();

    // Make request
    makeRequest("/codice/MO2076");

    // Check HTTP status
    checkHttpStatus();

    // Skip HTTP headers
    skipHttpHeaders();

    // Allocate the JSON document
    // Use arduinojson.org/v6/assistant to compute the capacity.
    const PROGMEM size_t capacity = 100;
    //  JSON_OBJECT_SIZE(2) + 40;

    StaticJsonDocument<capacity> PROGMEM doc;

    // Parse JSON object
    DeserializationError error = deserializeJson(doc, client);
    if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.c_str());
        return;
    }

    lcd.clear();

    strcpy(nomeFermata, doc["nome"]);
    strcpy(oraAttuale, doc["orario"]);
    //  nomeFermata = doc["nome"];
    //  Calcola centro
    Serial.println(F("Nome fermata:"));
    Serial.println(nomeFermata);
    Serial.println(F("Orario:"));
    Serial.println(oraAttuale);
    lcd.setCursor(cursoreFermata(), 0);
    lcd.print(nomeFermata);
    lcd.setCursor(5, 1);
    lcd.print(oraAttuale);

    // Disconnect
    client.stop();

    delay(2000);
}

void loop() {
    // DEBUG
    Serial.println(F("mo mi connetto al server"));
    // Connect to HTTP server
    connectToServer();
    // DEBUG
    Serial.println(F("OK! ora aspettiamo POI parte richiesta"));
    delay(1000);

    // DEBUG
    // Serial.println(F("ok, ora richiesto"));

    // Make request
    //  makeRequest("/codice/MO2076");
    // Do this until URL is fixed
    // DEBUG
    Serial.println(F("ora FACCIAMO RICHIESTA"));
    client.println(F("GET /corse/mo/MO2076?corseFermataOpposta=si HTTP/1.0"));
    client.println(F("Host: setaapi2.bitrey.it"));
    client.println(F("Connection: close"));
    if (client.println() == 0) {
        Serial.println(F("Failed to send request"));
        return;
    }
    // DEBUG
    Serial.println(F("FATTO!! wait poi check http status"));
    delay(1000);

    // DEBUG
    // Serial.println(F("checking http status!"));

    // Check HTTP status
    checkHttpStatus();
    // DEBUG
    Serial.println(F("CONTROLLATO, ora wait poi headers"));
    delay(1000);

    // DEBUG
    // Serial.println(F("skipping http headers!"));

    // Skip HTTP headers
    skipHttpHeaders();
    // DEBUG
    Serial.println(F("fatto, poi allocazione"));
    delay(1000);

    // DEBUG
    // Serial.println(F("clearing lcd!"));

    // Allocate the JSON document
    // Use arduinojson.org/v6/assistant to compute the capacity.
    const PROGMEM size_t capacity = 600;

    StaticJsonDocument<capacity> PROGMEM doc;

    // Parse JSON object
    DeserializationError error = deserializeJson(doc, client);
    if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.c_str());
        return;
    }

    // DEBUG
    // Serial.println(F("allocated, now showing"));

    // Viene impostato a vero se qualche autobus viene stampato
    bool noBuses = true;
    // DEBUG
    Serial.println(F("allocato, ora loopo e clearo lcd"));
    delay(1000);

    lcd.clear();

    JsonArray arr = doc.as<JsonArray>();
    for (int i = 0; i < arr.size(); i++) {
        JsonObject bus = arr[i];

        const char PROGMEM *linea = bus["linea"];
        const char PROGMEM *destinazione = bus["destinazione"];
        const char PROGMEM *pianificato = bus["pianificato"];
        const char PROGMEM tipoLinea = bus["tipo_linea"];
        const int PROGMEM minArrivo = bus["min_all_arrivo_val"];
        const char PROGMEM *tempoReale = bus["temporeale"];

        if (!linea || strlen(linea) == 0) { /* continue ; */
            //      DEBUG
            Serial.println("No linea");
        }

        noBuses = false;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(linea);
        //    Serial.println(strlen(destinazione));
        lcd.setCursor(strlen(linea) + 1, 0);
        //    const char destSubstr[16];
        //    strncpy(destSubstr, destinazione, strlen(linea) + 15 -
        //    strlen(destinazione));
        lcd.print(destinazione);
        if (minArrivo != 9999) {
            lcd.setCursor(0, 1);
            lcd.print(tempoReale);
            lcd.print(" - ");
            lcd.print(minArrivo);
            lcd.print(" min");
        } else {
            lcd.setCursor(0, 1);
            lcd.print(pianificato);
            lcd.print(" (pianific)");
        }
        //    lcd.setCursor(6, 1);
        //    lcd.print("Real");
        //    lcd.setCursor(11, 1);
        //    lcd.print(minArrivo != 9999 ? tempoReale : "N/A");

        Serial.println(F("*************"));
        Serial.println(F("Linea:"));
        Serial.println(linea);
        //    Serial.println(sizeof(linea));
        Serial.println(F("Destinazione:"));
        Serial.println(destinazione);
        Serial.println(F("Pianificato:"));
        Serial.println(pianificato);
        if (minArrivo != 9999) {
            Serial.println(F("Tempo reale:"));
            Serial.println(tempoReale);
        } else {
            Serial.println(F("Tempo reale:"));
            Serial.println("sconosciuto");
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
        // DEBUG
        Serial.println(F("Nessuna corsa"));
    }

    // DEBUG
    Serial.println(F("Dimensione array"));
    Serial.println(arr.size());

    delay(1000);

    // Disconnect
    client.stop();

    doc.clear();
}
