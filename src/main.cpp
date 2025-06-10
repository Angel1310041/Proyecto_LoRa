#include "Pantalla.h"
#include "interfaz.h"
#include "TransmisorRf.h"
#include "main.h"
#include <RCSwitch.h>
#include <heltec.h> 

String Version = "3.1.2.1";
const int EEPROM_SIZE = 512;

boolean debug = true, variableDetectada = false, modoprog = false; 
SENSOR activo{-1, -1, -1};

const int BOTON_PRUEBA_PIN = 2;
unsigned long tiempoUltimaImagen = 0;
int imagenMostrada = 1; 

extern String mensajePendiente;
extern bool enviarLoraPendiente;

bool animacionIniciada = false;
bool animacionInhabilitada = false;  // Nueva variable
unsigned long tiempoInicioAnimacion = 0;
int frameAnimacion = 0;

extern const unsigned char img1[], img2[], img3[], img4[], img5[], img6[], img7[], img8[], img9[], img10[], img11[], img12[], img13[], img14[];

void imprimir(String m, String c) {
  if (!debug) return;
  const char* col = "\033[0m";
  if (c == "rojo") col = "\033[31m";
  else if (c == "verde") col = "\033[32m";
  else if (c == "amarillo") col = "\033[33m";
  else if (c == "cyan") col = "\033[36m";
  Serial.print(col); Serial.println(m); Serial.print("\033[0m");
}

void animarAvance() {
  if (animacionInhabilitada) return;

  unsigned long ahora = millis();
  if ((ahora - tiempoInicioAnimacion) >= 300) {
    Heltec.display->clear();

    switch (frameAnimacion % 3) {
      case 0: Heltec.display->drawXbm(0, 0, 128, 64, img12); break;
      case 1: Heltec.display->drawXbm(0, 0, 128, 64, img13); break;
      case 2: Heltec.display->drawXbm(0, 0, 128, 64, img14); break;
    }

    Heltec.display->display();
    frameAnimacion++;
    tiempoInicioAnimacion = ahora;
  }
}


void enviarPorLora(String mensaje) {
  LoRa.beginPacket();
  LoRa.print(mensaje);
  LoRa.endPacket();
  Serial.println("Lora enviado: " + mensaje);
}

void mostrarImagen(const unsigned char* imagen, int tipo = 2) {
  Heltec.display->clear();
  Heltec.display->drawXbm(0, 0, 128, 64, imagen);
  Heltec.display->display();
  if (tipo == 1) {
    imagenMostrada = 1;
  } else {
    imagenMostrada = 2;
    tiempoUltimaImagen = millis();
  }
}

void mostrarInicio() {
  mostrarImagen(img1, 1);
  tiempoUltimaImagen = millis();
  animacionIniciada = false;
  animacionInhabilitada = false; // Reanudar animación
}

void mostrarImagenPorTipoSensor(int tipoSensor) {
  switch (tipoSensor) {
    case 0:
    case 1: mostrarImagen(img3); break;
    case 2: mostrarImagen(img5); break;
    case 3:
    case 4: mostrarImagen(img8); break;
    case 5:
    case 6: mostrarImagen(img11); break;
    case 7: mostrarImagen(img7); break;
    case 9: mostrarImagen(img2); break;
    default: break;
  }
}

void mostrarPantallaPorNumero(int numero) {
  Serial.printf("Mostrando contenido para pantalla numero: %d\n", numero);
  if (!Heltec.display) {
    Serial.println("Error: Display no inicializado.");
    return;
  }
  Heltec.display->clear(); 
  switch (numero) {
    case 1: Heltec.display->drawXbm(0, 0, 128, 64, img3); break;
    case 2: Heltec.display->drawXbm(0, 0, 128, 64, img5); break;
    case 3: Heltec.display->drawXbm(0, 0, 128, 64, img8); break;
    case 4: Heltec.display->drawXbm(0, 0, 128, 64, img10); break;
    case 5: Heltec.display->drawXbm(0, 0, 128, 64, img9); break;
    case 6: Heltec.display->drawXbm(0, 0, 128, 64, img11); break;
    case 7: Heltec.display->drawXbm(0, 0, 128, 64, img7); break;
    default:
      Heltec.display->drawString(0, 0, "Pantalla Invalida");
      Serial.printf("Numero de pantalla invalido recibido: %d\n", numero);
      break;
  }
  Heltec.display->display(); 
}

void blinkLed() {
  for (int i = 0; i < 2; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(500);
    digitalWrite(LED_PIN, LOW);
    delay(500);
  }
}

void manejarEntradas() {
  static unsigned long t = 0, progStart = 0;
  static bool esperandoLiberar = false;
  static bool botonAnterior = HIGH;

  int lecturaSensor = digitalRead(MQ6_PIN);
  int progEstado = digitalRead(prog);
  int estadoBoton = digitalRead(BOTON_PRUEBA_PIN);

  if (progEstado == LOW) {
    if (!progStart) progStart = millis();
    if (!modoprog && millis() - progStart >= 2000 && !esperandoLiberar) {
      modoprog = esperandoLiberar = true;
      entrarmodoprog();
      animacionInhabilitada = true; 
      Heltec.display->clear();
      Heltec.display->drawXbm(30, 0, 70, 40, img4);
      Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
      Heltec.display->setFont(ArialMT_Plain_10);
      Heltec.display->drawString(0, 52, "IP: 192.168.8.28");
      Heltec.display->display();
      imprimir("Entrando en modo programación...", "cyan");
    }
  } else {
    progStart = 0;
    esperandoLiberar = false;
  }

  if (!modoprog) {
    if (millis() - t > 10000) {
      imprimir("Lectura Sensor (" + String(lecturaSensor) + ")");
      t = millis();
    }

    bool sensorActivo = false;
    if (activo.tipo == 0) sensorActivo = (lecturaSensor == LOW);
    else sensorActivo = (lecturaSensor == HIGH);

    if (sensorActivo && !variableDetectada) {
      int mensajeRF = (activo.id * 10000) + (activo.tipo * 1000) + activo.zona;
      Transmisorrf.send(mensajeRF, 32);
      mostrarImagenPorTipoSensor(activo.tipo);
      blinkLed();
      variableDetectada = true;
      animacionInhabilitada = true; // Desactivar animación
      imprimir("Alerta RF enviada: " + String(mensajeRF), "rojo");
    } else if (!sensorActivo) {
      variableDetectada = false;
    }

    if (estadoBoton == LOW && botonAnterior == HIGH) {
      if (activo.id != -1 && activo.zona != -1) {
        int mensajeRF = (activo.id * 10000) + (9 * 1000) + activo.zona; 
        Transmisorrf.send(mensajeRF, 32);
        blinkLed();
        mostrarImagen(img2);
        animacionInhabilitada = true; // Desactivar animación
        imprimir("Señal RF enviada con datos registrados: " + String(mensajeRF), "verde");
      } else {
        imprimir("Error: No hay datos registrados en EEPROM", "rojo");
      }
    }
    botonAnterior = estadoBoton;
  }

 if (!modoprog && imagenMostrada == 2 && millis() - tiempoUltimaImagen >= 10000) {
  imagenMostrada = 1;
  animacionIniciada = true;
  animacionInhabilitada = false;  // ← AÑADE ESTA LÍNEA
  frameAnimacion = 0;
  tiempoInicioAnimacion = millis();
}


}

void procesarEnvioLora() {
  if (enviarLoraPendiente && mensajePendiente.length() > 0) {
    enviarPorLora(mensajePendiente);
    mensajePendiente = "";
    enviarLoraPendiente = false;
  }
}

void setup() {
  Serial.begin(115200);
  EEPROM.begin(EEPROM_SIZE);

  pinMode(MQ6_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);  
  digitalWrite(LED_PIN, LOW);
  pinMode(prog, INPUT_PULLUP);
  pinMode(BOTON_PRUEBA_PIN, INPUT_PULLUP);

  Heltec.begin(true, false, true);
  mostrarInicio();

  Transmisorrf.enableTransmit(33);
  EEPROM.get(0, activo);

  if (activo.id < 1000 || activo.id > 9999 ||
      activo.zona < 1 || activo.zona > 510 ||
      activo.tipo < 0 || (activo.tipo > 7 && activo.tipo != 9)) 
  {
    imprimir("Datos EEPROM inválidos o fuera de rango, restaurando...", "amarillo");
    activo = {0, 0, 0}; 
    EEPROM.put(0, activo); 
    EEPROM.commit();
  }

  int lecturaSensor = digitalRead(MQ6_PIN);
  bool sensorActivo = (activo.tipo == 0) ? (lecturaSensor == LOW) : (lecturaSensor == HIGH);
  if (sensorActivo) variableDetectada = true;

  imprimir("ID: " + String(activo.id));
  imprimir("Zona: " + String(activo.zona));
  imprimir("Tipo: " + String(activo.tipo));
}

void loop() {
  manejarEntradas();
  procesarEnvioLora();

  if (!modoprog && !animacionInhabilitada && !animacionIniciada && imagenMostrada == 1 && millis() - tiempoUltimaImagen >= 10000) {
    animacionIniciada = true;
    frameAnimacion = 0;
    tiempoInicioAnimacion = millis();
  }

  if (!animacionInhabilitada && animacionIniciada) {
    animarAvance();
  }
}
