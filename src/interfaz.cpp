#include "main.h"
#include "interfaz.h"
#include "TransmisorRf.h"
#include "Pantalla.h"
#include <SPIFFS.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <esp_system.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Ticker.h>


IPAddress local_IP(192, 168, 8, 28);
IPAddress gateway(192, 168, 8, 1);
IPAddress subnet(255, 255, 255, 0);

const char* ssidAP = "SENSOR-ABM";
const char* passwordAP = "12345678";

AsyncWebServer server(80);
Ticker restartTimer;
extern String Version;

String mensajePendiente = "";
bool enviarLoraPendiente = false;

void animacionCarga() {
    const char* estados[] = {"-", "\\", "|", "/"};
    for (int i = 0; i < 10; i++) {
        Serial.print("\rCargando... ");
        Serial.print(estados[i % 4]);
        vTaskDelay(200 / portTICK_PERIOD_MS);
    }
    Serial.println("\rCargando... ¡Listo!");
}

void procesarArchivoJSON(const char* path, AsyncWebServerRequest* request) {
    DynamicJsonDocument doc(256);
    doc["version"] = Version;

    File file = SPIFFS.open(path, "r");
    if (!file) {
        imprimir("No se pudo abrir el archivo JSON para leer");
        doc["error"] = "No se pudo abrir el archivo";
        String respuesta;
        serializeJson(doc, respuesta);
        request->send(500, "application/json", respuesta);
        return;
    }

    DeserializationError error = deserializeJson(doc, file);
    file.close();
    if (error) {
        imprimir("Error al parsear el JSON");
        doc["error"] = "JSON no válido";
        String respuesta;
        serializeJson(doc, respuesta);
        request->send(400, "application/json", respuesta);
        return;
    }

    imprimir("JSON recibido y procesado correctamente");
    doc["mensaje"] = "Archivo recibido y procesado";
    String respuesta;
    serializeJson(doc, respuesta);
    request->send(200, "application/json", respuesta);
}

void programarReinicio() {
    restartTimer.once(1.0, []() {
        ESP.restart();
    });
}
void endpointsMProg(void *pvParameters) {
    animacionCarga();
    WiFi.mode(WIFI_AP);
    if (!WiFi.softAPConfig(local_IP, gateway, subnet)) {
        imprimir("Error al configurar IP estática");
        vTaskDelete(nullptr);
        return;
    }

    if (!WiFi.softAP(ssidAP, passwordAP, 6, 0, 4)) {
        imprimir("Error al iniciar el AP");
        vTaskDelete(nullptr);
        return;
    }
    imprimir("Punto de acceso creado: " + WiFi.softAPIP().toString());
    
        
    server.on("/programacion", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", "Modo Programación Activado");
    });

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (!SPIFFS.exists("/interfaz.html.gz")) {
            imprimir("Archivo /interfaz.html.gz no encontrado");
            request->send(404, "text/plain", "Archivo no encontrado");
            return;
        }

        auto *response = request->beginResponse(SPIFFS, "/interfaz.html.gz", "text/html");
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });

    server.on("/reiniciar", HTTP_POST, [](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", "Reiniciando...");
        programarReinicio();
    });
    
    
    server.on("/guardar-parametros", HTTP_POST,
        [](AsyncWebServerRequest *request) {}, nullptr,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t, size_t) {
            DynamicJsonDocument doc(512);
            if (deserializeJson(doc, data, len)) {
                request->send(400, "application/json", "{\"error\": \"JSON inválido\"}");
                return;
            }

            if (!doc.containsKey("id-alarma") || !doc.containsKey("zona") || !doc.containsKey("tipo-sensor")) {
                request->send(400, "application/json", "{\"error\": \"Parámetros incompletos\"}");
                return;
            }

            int id = doc["id-alarma"];
            int zona = doc["zona"];
            int tipo = doc["tipo-sensor"];

            if (id < 1000 || id > 9999 || zona < 1 || zona > 512 || tipo < 0 || tipo > 9) {
                request->send(400, "application/json", "{\"error\": \"Parámetros fuera de rango\"}");
                return;
            }
            activo.id = id;
            activo.zona = zona;
            activo.tipo = tipo;
            if (EEPROM.put(0, activo), EEPROM.commit()) {
                request->send(200, "application/json", "{\"status\": \"Parámetros guardados\"}");
            } else {
                request->send(500, "application/json", "{\"error\": \"Error al guardar en EEPROM\"}");
            }
        });
    server.on("/mostrar-pantalla", HTTP_POST,
        [](AsyncWebServerRequest *request) {}, nullptr,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t, size_t) {
            DynamicJsonDocument doc(256);
            if (deserializeJson(doc, data, len)) {
                request->send(400, "application/json", "{\"error\": \"JSON inválido\"}");
                return;
            }
            if (!doc.containsKey("numero")) {
                request->send(400, "application/json", "{\"error\": \"Falta el parámetro 'numero'\"}");
                return;
            }
            mostrarPantallaPorNumero(doc["numero"]);
            request->send(200, "application/json", "{\"status\": \"Pantalla mostrada\"}");
        });

    server.on("/enviar-lora", HTTP_POST,
        [](AsyncWebServerRequest *request) {
            request->send(400, "application/json", "{\"error\": \"Falta el cuerpo del mensaje\"}");
        },
        nullptr,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t, size_t) {
            DynamicJsonDocument doc(256);
            if (deserializeJson(doc, data, len)) {
                request->send(400, "application/json", "{\"error\": \"JSON inválido\"}");
                return;
            }
            String mensaje = doc["mensaje"] | "";
            if (mensaje.isEmpty()) {
                request->send(400, "application/json", "{\"error\": \"Falta el campo 'mensaje'\"}");
                return;
            }
            mensajePendiente = mensaje;
            enviarLoraPendiente = true;
            request->send(200, "application/json", "{\"status\": \"Mensaje recibido\"}");
        });
        
    server.on("/get-parametros", HTTP_GET, [](AsyncWebServerRequest *request) {
        DynamicJsonDocument doc(256);
        doc["id"] = activo.id;
        doc["zona"] = activo.zona;
        doc["tipo"] = activo.tipo;
        String json;
        serializeJson(doc, json);
        request->send(200, "application/json", json);
    });
    server.on("/enviar-rf-prueba", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (activo.id == 0) {
            request->send(400, "application/json", "{\"error\": \"No hay alarma registrada\"}");
            return;
        }
        int mensajeRF = activo.id * 10000 + 9000 + activo.zona;
        Transmisorrf.send(mensajeRF, 32);
        imprimir("Señal RF enviada: " + String(mensajeRF), "verde");
        request->send(200, "application/json", "{\"status\": \"Señal RF enviada\"}");
    });

    server.begin();
    vTaskDelay(100 / portTICK_PERIOD_MS);
    vTaskDelete(nullptr);
}

void entrarModoProgramacion() {
    imprimir("Entrando a modo programación...");
    modoprog = true;
    digitalWrite(LED_PIN, HIGH);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    imprimir("Activando Modo Programación...");
    xTaskCreatePinnedToCore(
        endpointsMProg,
        "endpoints",
        8192,
        nullptr,
        2,
        nullptr,
        1
    );
    imprimir("---# Modo Programación Activado #---", "verde");
}
void entrarmodoprog() {
    if (!SPIFFS.begin(true)) {
        imprimir("Error al montar SPIFFS");
        return;
    }
    entrarModoProgramacion();
}