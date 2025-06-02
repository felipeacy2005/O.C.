#include <WiFi.h> // Incluimos las librerias para conectarse a una red wifi
#include <WebServer.h> // Esta es la que responde a las solicitudes de la red
#include <HardwareSerial.h> // Para comunicarse con otra targeta mediante el UART, en este caso el arduino
#include <ArduinoJson.h>    // Como vamos a comunicar otro lenguaje de programación utilizamos el JSON para comunicar dos lenguajes
#include <DHT.h>            // Libreria del sensor de temperatura y humedad (investigar libreria, da curiosidad xd)

// Configuración de la red (Llevar router y configurar el router antes de presentar el trabajo para evitar fallos :D)
const char* ssid = "TCarmona";      // Colocamos el nombre de la red a la que queremos conectar
const char* password = "carmona15"; // Y despues colocamos la contraseña de la red para alojar el servidor más adelante

WebServer server(80); // Comando para iniciar el servidor en la red, en este caso el 80 (los demás están ocupados)
// Habilitamos la comunicacion serial de los UART de la ESP-32 (Rx2 t Tx2)
HardwareSerial SerialArduino(2); //Pines 16 y 17 de la placa, es como si las dos placas se hablaran

// Todas las variables usadas por si queremos cambiar los pines (aprendí a las malas)
float lastHumidity = 0.0;
float lastTemperature = 0.0;

// --- CONFIGURACIÓN SERIAL UART ---
// Definimos los pines que mencioné anteriormente y les damos un nombre para identificarlos
const int ARDUINO_RX_PIN = 16;
const int ARDUINO_TX_PIN = 17;
// Definimos los pines que utilizaré para el módulo RGB
const int LED_RED_PIN = 4;   // Pin para el color rojo
const int LED_GREEN_PIN = 5; // Pin para el color verde 
const int LED_BLUE_PIN = 18; // Pin para el color azul, facil

// Canales para no interferir uno con el otro, son PWM para variar el brillo 
const int LEDC_CHANNEL_RED = 0;
const int LEDC_CHANNEL_GREEN = 1;
const int LEDC_CHANNEL_BLUE = 2;
// Frecuencia y resolución del PWM 8 bits = 0-255 (para regular la luz RGB)
const int LEDC_FREQ = 5000;      // Frecuencia de 5 hercios 
const int LEDC_RESOLUTION = 8;   // Con una precisión de 8 bits osea, 256 niveles de brillo (HEX)
// Definición de los leds del pasillo
const int LED_SALA_PIN = 19; 
const int LED_PASILLO_PIN = 23; // parece que no sirve el 21 xd, mirar esto despues 
const int LED_COCINA_PIN = 22;
// Definimos las variables para saver le estado del led, ya sabemos que true y flase es 1 y 0;
bool salaState = false;
bool pasilloState = false;
bool cocinaState = false;

// Handler para la ruta principal, buenas prácticas
// empezamos con nuestra funciones, donde respondemos con un mensaje en la app para encontrar los datos del sensor
void handleRoot() {
  server.send(200, "text/plain", "ESP32 Sensor Gateway. Accede a /sensor para datos.");
}
// Si se pregunta por la informacion del sensor, se envia una respuesta con los datos
// esto en formato JSON
void handleSensorData() {
  String jsonResponse = "{";
  jsonResponse += "\"humidity\":" + String(lastHumidity) + ",";
  jsonResponse += "\"temperature\":" + String(lastTemperature);
  jsonResponse += "}";

  server.send(200, "application/json", jsonResponse);
}

// Definimos la función para el modulo RGB (lo llamaré así, ps tiene los colores RGB)
// Esta función es la que lee los datos en formato JSON y los transforma en una señal PWM
// osea lo que se envia del telefono en el POST por eso el metodo 'recibe' informacion y 
// la ejecuta de acuerdo a lo que quiero, en este caso la señal PWM para el módulo RBG
void handleSetColor() {
 
  if (server.method() == HTTP_POST) { 
  // esperando una petición POST con JSON si o si tiene que ser JSON.
  // de otro modo genera el error 400
    if (server.hasArg("plain")) {
      String requestBody = server.arg("plain");
      Serial.print("Cuerpo JSON recibido para /setcolor: ");
      Serial.println(requestBody);

      DynamicJsonDocument doc(256); // Se espera el tamaño indicado
      DeserializationError error = deserializeJson(doc, requestBody);

      if (error) {
        Serial.print(F("deserializeJson() falló: "));
        Serial.println(error.f_str());
        server.send(400, "text/plain", "JSON inválido");
        return;
      }

      // La parte bonita de esto.
      // aquí es donde si el aplicativo mandó un JSON y es correcto
      // se extrae la informacion de los colores 
      int r = doc["r"] | 0; // información del Rojo (Red = R)
      int g = doc["g"] | 0; // información del Verde (Green = G)
      int b = doc["b"] | 0; // información del Azul (Blue = B)
      // Verificamos si están dentro del rango extablecido anteriormente
      // osea, en el rango de 8 bits (256) para la señal PWM
      r = constrain(r, 0, 255);
      g = constrain(g, 0, 255);
      b = constrain(b, 0, 255);

      // Ahora la intensidad de los LEDs RGB será la que se envio y podemos mandarla como señal PWM
      // de la siguiente manera
      ledcWrite(LEDC_CHANNEL_RED, r);
      ledcWrite(LEDC_CHANNEL_GREEN, g);
      ledcWrite(LEDC_CHANNEL_BLUE, b);
      // se da un mensaje por el monitor serial y tambien se envia información 
      // con el dódigo 200, verificando que la información fue valida, dando un salto de linea para mostrar los colores
      Serial.printf("Color RGB establecido: R:%d, G:%d, B:%d\n", r, g, b);
      // enviamos el código 200 que dije antes para que la aplicación no llore
      server.send(200, "text/plain", "Color RGB establecido con éxito");
    } else {
      // detecta un error con el JSON, donde no fué válido el formato establecido
      // y se lo enviamos al servidor para informar a la aplicación
      server.send(400, "text/plain", "Cuerpo de la petición vacío o incorrecto");
    }
  } else {
    // si detecta otro error diferente, generamos un error, que se lo enviamos al servidor
    // donde el aplicativo lo transmita
    server.send(405, "text/plain", "Método no permitido - Solo POST es aceptado para /setcolor");
  }
} 

// Definimos una función (la misma para todos los 3 leds, es mejor hacer un for, pero no soy tan pro)
// si se ejecuta este métodolo, se envia una señal digital (lo más fácil) al pin que definimos anteriormente
void handleSalaOn() {
  digitalWrite(LED_SALA_PIN, HIGH); // Debería encender el primer led
  salaState = true; // la variable la cambiamos a true para que permanesca encendido
  server.send(200, "text/plain", "Sala ON"); // despues le enviamos la informacion al servidor
  // donde la app lo extraerá y leerá para visualizar
  Serial.println("Comando: Sala ON"); // imprimimos por el monitor serial  que se encendio la sala
}
// la misma vaina pero con Low que puede ser 0, el boolean a false y lo demas se cambia de off a on
// y así continuamos con los demás
void handleSalaOff() {
  digitalWrite(LED_SALA_PIN, LOW);
  salaState = false;
  server.send(200, "text/plain", "Sala OFF");
  Serial.println("Comando: Sala OFF");
}
void handlePasilloOn() {
  digitalWrite(LED_PASILLO_PIN, HIGH);
  pasilloState = true;
  server.send(200, "text/plain", "Pasillo ON");
  Serial.println("Comando: Pasillo ON");
}
void handlePasilloOff() {
  digitalWrite(LED_PASILLO_PIN, LOW);
  pasilloState = false;
  server.send(200, "text/plain", "Pasillo OFF");
  Serial.println("Comando: Pasillo OFF");
}
void handleCocinaOn() {
  digitalWrite(LED_COCINA_PIN, HIGH);
  cocinaState = true;
  server.send(200, "text/plain", "Cocina ON");
  Serial.println("Comando: Cocina ON");
}
void handleCocinaOff() {
  digitalWrite(LED_COCINA_PIN, LOW);
  cocinaState = false;
  server.send(200, "text/plain", "Cocina OFF");
  Serial.println("Comando: Cocina OFF");
}

// Función para que la app sepa los estados actuales de los leds,
// si se apaga y enciende la ESP-32 obviamente todos están apagados xd
// lo definimos antes en las variables booleanas con true
void handleLedStatus() {
  // de igual forma se genera un JSON para enviar los estados de los leds
  // es decir 0 o 1 en pocas palabras
  DynamicJsonDocument doc(128); 
  doc["sala"] = salaState;
  doc["pasillo"] = pasilloState;
  doc["cocina"] = cocinaState;
  String jsonResponse;
  serializeJson(doc, jsonResponse);
  // se envia la respuesta en formato JSON al servidor y
  // imprimimos por pantalla la solicitud
  server.send(200, "application/json", jsonResponse);
  Serial.println("App solicitó estado de LEDs.");
}
// En el void setup, preparamos todo aquello, como los pines, como se iniciarán, entrada/salida, en 0 o en 1, etc
void setup() {
  Serial.begin(115200); // Monitor serial para ver mensajes en pantalla
  Serial.println("ESP32 iniciando..."); // mensaje que inicia, se ve bien, pero es nada

  // habilitamos la comunicacion UART de arduino, con el mismo numero de baudios del arduino
  // si estubiera en otra comunicacion mandaría simbolos
  SerialArduino.begin(9600, SERIAL_8N1, ARDUINO_RX_PIN, ARDUINO_TX_PIN);
  // enviamos mensaje por pantalla
  Serial.println("Serial2 iniciado para comunicación con Arduino.");

  // Configuración de los pines para el LED RGB (PWM) 8 bits (256))
  ledcSetup(LEDC_CHANNEL_RED, LEDC_FREQ, LEDC_RESOLUTION);
  ledcAttachPin(LED_RED_PIN, LEDC_CHANNEL_RED);
  ledcSetup(LEDC_CHANNEL_GREEN, LEDC_FREQ, LEDC_RESOLUTION);
  ledcAttachPin(LED_GREEN_PIN, LEDC_CHANNEL_GREEN); 
  ledcSetup(LEDC_CHANNEL_BLUE, LEDC_FREQ, LEDC_RESOLUTION);
  ledcAttachPin(LED_BLUE_PIN, LEDC_CHANNEL_BLUE);

  // Iniciar LEDs RGB apagados al principio (0 = low, 1 = high)
  ledcWrite(LEDC_CHANNEL_RED, 0);
  ledcWrite(LEDC_CHANNEL_GREEN, 0);
  ledcWrite(LEDC_CHANNEL_BLUE, 0);
  // mandamos mensaje por pantalla
  Serial.println("LEDs RGB inicializados (apagados).");

  // validamos los pines como salida, pues solo recibe 0 o 1
  // y nos aseguramos que inicien siempre apagados
  pinMode(LED_SALA_PIN, OUTPUT);
  pinMode(LED_PASILLO_PIN, OUTPUT);
  pinMode(LED_COCINA_PIN, OUTPUT);
  digitalWrite(LED_SALA_PIN, LOW);
  digitalWrite(LED_PASILLO_PIN, LOW);
  digitalWrite(LED_COCINA_PIN, LOW);
  // me gusta imprimir los pasos por pantalla :D
  // buenas prácticas
  Serial.println("LEDs individuales inicializados (apagados).");
  // Enviamos un mensaje a la red que estamos conectando
  // y con el wifi.begin enviamos el nombre de la red y contraseña
  Serial.print("Conectando a ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  // definimos una variable que actuará como contador de segundos
  int attempts = 0;
  // ciclo para seguir intentando conectar por 30 segundos
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(1000);
    Serial.print(".");
    attempts++;
  }
// si logramos conectar, imprimimos la dirección IP del servidor local
// es donde estará nuestra comunicación con la app movil
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi conectado");
    Serial.print("Dirección IP de la ESP32: ");
    Serial.println(WiFi.localIP());

  }
  // si fallo la conexión reiniciamos la ESP-32 hasta que conecte xd
  // PD: revisar contraseña si pasa esto xd
   else {
    Serial.println("\n¡Fallo al conectar al WiFi! Verifica SSID/Contraseña.");
    Serial.println("Reiniciando ESP32...");
    ESP.restart();
  }


  // definicmos nuestras socitudes GET (Traer) para recibir informacion de la app y reflejarla en la maqueta
  // y tambien enviar los datos del sensor xd, lo definí como GET la primera xd
  // está el comando, el tipo de solicitud, y la función que se ejecutará
  server.on("/", HTTP_GET, handleRoot);
  server.on("/sensor", HTTP_GET, handleSensorData);
  server.on("/setcolor", HTTP_POST, handleSetColor);
  server.on("/sala_on", HTTP_GET, handleSalaOn);
  server.on("/sala_off", HTTP_GET, handleSalaOff);
  server.on("/pasillo_on", HTTP_GET, handlePasilloOn);
  server.on("/pasillo_off", HTTP_GET, handlePasilloOff);
  server.on("/cocina_on", HTTP_GET, handleCocinaOn);
  server.on("/cocina_off", HTTP_GET, handleCocinaOff);
  server.on("/led_status", HTTP_GET, handleLedStatus); 



  // correccion de errores inesperados al conectar con el servidor
  // donde si no se conecta (puede que esté ocupado o se ocupe despues de un rato) enviaremos un error
  // en este caso enviaremos (404 Not Found)
  server.onNotFound([]() {
    Serial.print("404 Not Found: ");
    Serial.println(server.uri());
    server.send(404, "text/plain", "Ruta no encontrada: " + server.uri());
  });
  // si no pasa nada, iniciamos servidor local y damos un mensaje
  server.begin();
  Serial.println("Servidor HTTP iniciado.");
}

void loop() {
  // como vamos a manejar la información del sensor del arduino, tenemos que precesar la informacion 
  server.handleClient(); // solicitudes HTTP

  // Lee los datos del Arduino si están disponibles en su monitor serial aparte 
  // por eso es importante los baudios del Arduino
  if (SerialArduino.available()) {
    String data = SerialArduino.readStringUntil('\n'); // Leemos lo que esta en el monitor serial del arduino
    data.trim(); // función que elimina los espacios de un string
    // mensaje de confimación de los datos y seguido de los datos del sensor xd 
    Serial.print("Datos recibidos de Arduino: ");
    Serial.println(data);

    // buscamos las letras clave en la cadena de string que manda el arduino,
    // En este caso buscamos H osea Humendad del sensor y le indicamos donde terminan esos datos
    // osea en ,T: y seguimos con la temperatura, lo que esta antes de los datos
    // para que al final leamos el final de la cadena.
    // si estos datos se leen mal o no se encutran los datos, por defecto coloca -1
    int h_start = data.indexOf("H:");
    int h_end = data.indexOf(",T:");
    int t_start = data.indexOf("T:");
    int t_end = data.length();

    // si todos los datos son diferentes de -1 (osea, se recibieron los datos correctamente)
    // se ejecutará lo siguiente: 
    if (h_start != -1 && h_end != -1 && t_start != -1 && t_end > t_start) {
      // definimos 2 variables de temperature y humidity, que es igual a la cadena
      // pero empezando por el indice 2, porque solo queremos los datos, y al final 
      // indicamos donde acaba la cadena
      String humidityStr = data.substring(h_start + 2, h_end);
      String temperatureStr = data.substring(t_start + 2, t_end);
      // y ahora si utilizamos las variables definicas al principio para guardar los datos
      // no sin antes hacer un casting a float, para una mejor precisión
      lastHumidity = humidityStr.toFloat();
      lastTemperature = temperatureStr.toFloat();
      // imprimimos los resultados con printf f de (float) y le indicamos el formato
      // donde %.1f es un número flotante con un decimal
      Serial.printf("Humedad: %.1f%%, Temperatura: %.1f°C\n", lastHumidity, lastTemperature);
    } else {
      // si se recibió un -1 en algunos de los datos del arduino, nos dará un mensaje de error
      // y lo que se está recibiendo del arduino
      Serial.println("Formato de datos inválido del Arduino.");
      Serial.print("Recibido: ");
      Serial.println(data);
    }
    // se termina el void loop() y se ejecuta una y otra vez
  }
}
