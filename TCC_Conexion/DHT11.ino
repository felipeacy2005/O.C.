// Incluimos las librerias para ahorar código
#include <DHT.h>
#include <DHT_U.h>
// Definimos el tipo de sensor que tenemos
// hay muchos tipos de sensores en el mercado, pero
// en este caso tenemos el DHT11
#define DHTTYPE DHT11
// y definimos las variables de temperatura y humedad
float h;
float t;
// Definimos las variables para leer los datos del pin S del sensor
// y tambien el buzer que se activará cuando la temperatura sea mayor a 38 grados
int dhtpin = 2;
int pinbuzzer = 13;
// utilizamos la palabra recerbada para instanciar el nombre del sensor y el pin 
// que vamos a usar
DHT dht(dhtpin, DHTTYPE);
// definimos un delay para ajustar los tiempos
// esto es igual a 3 segundos
int dt = 3000;


void setup() {
  // Configura el pin del buzzer como salida para enviarle señal alta o baja
  pinMode(pinbuzzer, OUTPUT);     
  // Asegura que el buzzer esté apagado al inicio, aveces pita xd
  digitalWrite(pinbuzzer, LOW);   

  // iniciamos el monitor serial en 9600 baudios, pero podemos colocarlos en el que
  // queramos, esto es ver los datos que estaremos enviado
  Serial.begin(9600);
  // iniciamos el sensor
  dht.begin();
}

void loop() {
  h = dht.readHumidity();   // Lee la humedad.
  t = dht.readTemperature(); // Lee la temperatura en Celsius.

  // esto nos nostrará si el sensor está funcionando mal y no
  // está leyendo los datos
  if (isnan(h) || isnan(t)) {
    Serial.println("No hay señal del sensor");
    delay(3000); 
    return;      
  }

  // Hacemos que el buzzer se active con un if dependiendo de la temperatura
  // donde si es mayor a 38 ( ejmp: 38.1) se active el buzer y si no, que permanezaca 
  // desactivado
  if (t > 38.0) { 
    // damos un mensaje por pantalla, mostramos la temperatura
    // y luego mandamos una señal al buzer para que encienda,
    // espera 3 segundos y luego se apague.
    Serial.print("Temperatura alta, ¡algo se quema!");
    Serial.print(t);
    digitalWrite(pinbuzzer, HIGH); 
    delay(dt);                  
    digitalWrite(pinbuzzer, LOW);
  } else {
    digitalWrite(pinbuzzer, LOW);
  }

  // colocaré H y T para no utilizar muchas funciones
  // Esta es Humedad
  Serial.print("H:");
  Serial.print(h);
  // Esta es la temperatura en °C
  Serial.print(",T:");
  Serial.println(t);

  // damos el delay que 3 segundos
  delay(dt);
}
// Fin :D