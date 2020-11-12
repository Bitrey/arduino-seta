# SETA Quanto Manca per Arduino

Questo mio primo progetto di Arduino utilizza la API (non ufficiale, fatta da me appositamente per questo) che trovi a [questo link](https://github.com/Bitrey/SETA-API-V2 "SETA Quanto Manca API v2").

Per cambiare fermata, basta cambiare la variabile `const char *CODICE_FERMATA` assengnandole il codice della fermata desiderata, puoi trovare i codici [qua](https://github.com/Bitrey/SETA-API-V2/blob/main/fermate.json "Codici fermate di Modena").

Utilizza le seguenti librerie:

- **ArduinoJson** in quanto l'API risponde in formato JSON.
- **Ethernet** per connettersi ad internet, assieme alla libreria integrata **SPI**.
- **LiquidCrystal_I2C** per stampare le risposte su uno schermo LCD 2x16.

Eventualmente potrebbe essere aggiornato in modo da funzionare su un display LED matrix.

Testato e funzionante su Arduino Uno. Se crasha, prova ad aumentare la capacity (la variabile `const size_t capacity` nella funzione loop) ad un valore più alto, poi calcolare il valore preciso [qua](https://arduinojson.org/v6/assistant/ "Assistente Arduino JSON").
