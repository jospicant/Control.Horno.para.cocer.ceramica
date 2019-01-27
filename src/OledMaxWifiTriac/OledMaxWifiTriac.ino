
// *****************  Status:OK probado con Placa NodeMCU 1.0 ESP12E  ***********************************++++

// Incluir la librería ESP8266WiFi **********************************
#include <ESP8266WiFi.h>

//*******************************************  Wifi conexión ESP a Router **********************************************
IPAddress ip,gateway,mask; 
char ssid[] = "yourSSID"; // También valdría string ssid[] = "nombre_wifi";
char pass[] = "yourPassW"; // También valdría string pass[] = "contraseña_wifi";
boolean conectado=false;
//**********************************************************************************************************************


//********************************************* Datos thingSpeak  y Cte Wifi *******************************************

WiFiClient clienteThingSpeak;   // Objeto Cliente de ThingSpeak
boolean ThingSpeakON;

char direccionThingSpeak[] = "api.thingspeak.com";
String APIKey = "yourAPIKEYdeThingSpeak";
int muestreo = 20 * 1000; //  Enviaremos datos a un canal determinado de ThingSpeak cada 20 sg 
long tiempoActual,tiempoAnterior;

//**********************************************************************************************************************


//****************************************** Datos Oled ************************************************************
#include <Wire.h>
#include <SSD1306.h>
#define flipDisplay true
SSD1306 display(0x3c, D1, D2); //GPIO 5 = D1, GPIO 4 = D2    I2c  dirección 0x3c       D1 = SDA   D2= SCL
//**********************************************************************************************************************


//****************************************** Max31856  ****************************************************************
#include <Adafruit_MAX31856.h>    
// Use software SPI: CS, DI, DO, CLK
Adafruit_MAX31856 maxthermo = Adafruit_MAX31856(D7,D6,D5,D0);    //   Utilizo como SPI Software.   D8 No se puede Usar
// use hardware SPI, just pass in the CS pin
//Adafruit_MAX31856 maxthermo = Adafruit_MAX31856(10);
const byte offset=6;                                          //Offset corrección temperatura
//********************************************************************************************************************


//********************************* Control Triac *********************************************************************
byte triac = D4;   // Señal de acivación del triac en el pin D4 (coincide con el led azul del ESP8266 ) 
const bool Apagado=false;
const bool Encendido=true;
int temp[] = {50,100,120,150,180,230,280,330,400,450,500,550,600,650,700,750,800,850,900,950,1000,900,800,750,650};

int Tpwm=20000;   // Periodo del PWM 20 sg.  Al ser byte ( 255 como máximo )
byte dutty=100;
float t1=0,t2=0;
bool estadoPWM=false;

// PID
int kp=0.8;  int ki=0.3;  int kd=1.8;   
int PID_p=0; int PID_i=0; int PID_d=0;
float target=temp[1];                                       // Tª objetivo
float PID_errorActual=0,PID_errorAnterior=0;                // Error Anterior y Actual respecto al valor objetivo marcado
int PID_valor=0;
float t1_pid=0,t2_pid=0;                 //muestreo pid      marcas de tiempo para controlar el muestreo PID cada TmuestreoPID ms
float TmuestreoPID=100;                 // Periodo de muestreo del PID


//*********************************************************************************************************************


void setup() {

  
  pinMode(triac,OUTPUT);              // Configuración pin de activación del triac.  El Triac lo activamos por el pin D4
  digitalWrite(triac,Apagado);

  Serial.begin(115200);

  maxthermo.begin();                  //************ Max *************************                
  maxthermo.setThermocoupleType(MAX31856_TCTYPE_S);         // Estoy usando una sonda termopar del tipo S

  display.init();                     //******************** display **************
  if (flipDisplay) display.flipScreenVertically();

  /* Mostramos la pantalla de bienvenida */
  display.clear();
  display.setFont(ArialMT_Plain_16);     // Tipo de fuente a usar
  display.drawString(10, 25, "Horno");   // Mensaje inicial
  display.display();
  delay(1000);

//************************************************************ Conexión ESP a Router Wifi **************************************************************************

   display.setFont(ArialMT_Plain_10);
   WiFi.begin(ssid, pass);
   int intentos=0;
 
  while (WiFi.status() != WL_CONNECTED & intentos < 100)    // Saldrá del bucle cuando conecte o después de unos 50 sg de intento de conexión
  {
    delay(500);
    intentos++;
    display.clear();
    display.drawString(10,10, "Connecting to:");
    display.drawString(10,30, WiFi.SSID());
    display.drawString(10+intentos,45," .");
    display.drawString(70,45,String(intentos));
    display.display();
    Serial.print("->");
    
  }
  if(intentos >= 100) {
    conectado=false;     
    display.clear();
    display.drawString(10, 10, "Wifi Not Connected to:");
    display.drawString(10, 35, WiFi.SSID());
    display.display();
    Serial.print("Imposible conectar a la red Wifi: "); Serial.println(ssid);
    }
  else {     // En el caso de que conecte bien con el router, intentaremos probar conexión con ThingSpeak
    conectado=true;
    display.clear();
    display.drawString(10, 10, "Wifi Conected to:");
    display.drawString(10, 35, WiFi.SSID());
    display.display();
    delay(2000);
    DatosDeLaConexion();
    delay(5000);
    ThingSpeakON = ConexionConThingSpeak();
    if(ThingSpeakON){                                // probamos que alcanzamos a ThingSpeak
    display.clear();
    display.drawString(10,25, "ThingSpeak ON");
    display.display();
    }else{
    display.clear();
    display.drawString(10,25, "ThingSpeak OFF");
    display.display();
    }
    delay(5000);
   
  }

  delay(2000);
  intentos=0;

  tiempoAnterior=millis();              // inicio cronómetro para envio a ThingSpeak
  t1=millis();                        // inicio cronómetro para PWM del triac
  t1_pid=millis();
  
} 

//********************************************************************** PROGRAMA PPAL ****************************************************************************************

void loop() {
  
  display.setFont(ArialMT_Plain_16);
  //Serial.print("Temperatura Horno: "); 
  //Serial.println(maxthermo.readThermocoupleTemperature()+ offset);
  
  float  valor=maxthermo.readThermocoupleTemperature()+ offset;    // Tª edl horno
  String temp= String(valor);
  
  //Datos para thingSpeak
  String datosThingSpeak = "field1=" + temp;

  tiempoActual=millis();
  if(tiempoActual-tiempoAnterior >= muestreo){   //Si han pasado los 20 sg para poder enviar a ThingSpeak
  tiempoAnterior=millis();
  EnviarDatosThingSpeak(datosThingSpeak);      //Envia y revisaremos si han pasado 20 sg con la función millis
  }

  //PID ***************************************

  t2_pid=millis();
  
  if( (t2_pid-t1_pid) >= TmuestreoPID){          //Solo recalculo el PID cada cierto tiempo ( Periodo de muestreo )

    t1_pid=millis();             //reinicio cronómetro cuenta del muestreo
    
    PID_errorActual=target-valor;      //Cálculo del ERROR
    PID_p=kp*PID_errorActual;          //calculo parte proporcional
    if( PID_errorActual > -3  &  PID_errorActual <3 ){        // Si el error de temperatura está + - 3 grados --> aplico parte integral
        PID_i=PID_i+(ki*PID_errorActual);
       }
    PID_d=kd*(PID_errorActual-PID_errorAnterior)/TmuestreoPID;
    PID_valor=PID_p+PID_d+PID_i;

    if (PID_valor < 0) {
    PID_valor=0;
    }else if (PID_valor > 100){
    PID_valor=100;
    }
    PID_errorAnterior=PID_errorActual;
   }else{
    t1_pid=t1_pid;  // No hago nada
  }

  //Control Temperatura a 100 grados   **************************************

    dutty=(byte)PID_valor;
    Serial.print("Cálculo Dutty desde el PID = "); Serial.println(dutty);
    bool estadoPWM = PWMtriac(Tpwm,dutty);   // El Cálculo del PID me dirá cada 100 ms que PWM debo aplicar al horno para llegar al Target deseado
    digitalWrite(triac,estadoPWM);
   
   
   // Nota: He apreciado que con un PWM 5% sobre T=20sg  ( 1sg) la temperatura se llega a estabilizar y al final incluso empieza a subir un poco si mantenemos ese dutty
   // así durante media hora  ( tengo mi media hora estable para cambiar a otro punto de temperatura )
   
//***************************************************************************
  
  display.clear();
  display.drawString(10, 10 , "Temperatura: ");
  display.drawString(10, 35,temp);
  if(conectado){
    display.drawString(90,40,"O");
    if(ThingSpeakON) display.drawString(110,40,"O");
      else display.drawString(110,40,"X");
  }else{
    display.drawString(90,40,"X");
    if(ThingSpeakON) display.drawString(110,40,"O");
      else display.drawString(110,40,"X");
  }
  display.display();
  
  // Check and print any faults
  uint8_t fault = maxthermo.readFault();
  if (fault) {
    if (fault & MAX31856_FAULT_CJRANGE) Serial.println("Cold Junction Range Fault");
    if (fault & MAX31856_FAULT_TCRANGE) Serial.println("Thermocouple Range Fault");
    if (fault & MAX31856_FAULT_CJHIGH)  Serial.println("Cold Junction High Fault");
    if (fault & MAX31856_FAULT_CJLOW)   Serial.println("Cold Junction Low Fault");
    if (fault & MAX31856_FAULT_TCHIGH)  Serial.println("Thermocouple High Fault");
    if (fault & MAX31856_FAULT_TCLOW)   Serial.println("Thermocouple Low Fault");
    if (fault & MAX31856_FAULT_OVUV)    Serial.println("Over/Under Voltage Fault");
    if (fault & MAX31856_FAULT_OPEN)    Serial.println("Thermocouple Open Fault");
  }
  
 
}

//******************************************** Funciones Adicionales *************************************************************************


void DatosDeLaConexion(){            // Mostrar los datos de mi conexión.  SSID Wifi,IP,Gateway,máscara de red...

    ip=WiFi.localIP();
    gateway=WiFi.gatewayIP();
    mask=WiFi.subnetMask();
    
    
    String num_ip= String(ip[0])+"."+String(ip[1])+"."+String(ip[2])+"."+String(ip[3]);
    String mascara=String(mask[0])+"."+String(mask[1])+"."+String(mask[2])+"."+String(mask[3]);
    String ip_Gateway= String(gateway[0])+"."+String(gateway[1])+"."+String(gateway[2])+"."+String(gateway[3]);
    //Serial.print("ip: "); Serial.println(ip);
    
    display.clear();
    display.drawString(10,1,WiFi.SSID());
    display.drawString(10,15,num_ip);
    display.drawString(10,30,mascara);
    display.drawString(10,45,ip_Gateway);
    display.display();
}

 //*********************************** Establecemos la conexión del cliente con el servidor de ThingSpeak  ***********************************
boolean ConexionConThingSpeak(){     
   if (clienteThingSpeak.connect(direccionThingSpeak,80)) {
   return (true);
   }else
   return (false);
}

//************ Configuración del paquete a enviar a la plataforma ThingSpeak *****************************************************************
void EnviarDatosThingSpeak(String datos){
  
  // Establecemos la conexión con el cliente
   if (clienteThingSpeak.connect(direccionThingSpeak,80)) {           // No usar la función "ConexionConThingSpeak(), si se usa no fuciona 
    
  // Línea de comienzo del mensaje (Método URI versionprotocolo)
    clienteThingSpeak.println("POST /update HTTP/1.1");               //  api.thingspeak.com/update
 
    // Campos de cabecera
    clienteThingSpeak.print("Host: ");
    clienteThingSpeak.println(direccionThingSpeak);
    clienteThingSpeak.println("Connection: close");
    clienteThingSpeak.println("X-THINGSPEAKAPIKEY: " + APIKey);
    clienteThingSpeak.println("Content-Type: application/x-www-form-urlencoded");
    clienteThingSpeak.print("Content-Length: ");
    clienteThingSpeak.println(datos.length());
    clienteThingSpeak.println(); // Esta línea vacía indica que hemos terminado con los campos de cabecera
 
    // El cuerpo del mensaje, los datos
    clienteThingSpeak.print(datos);
 
    // Comprobamos si se ha establecido conexión con Thinspeak
    if (clienteThingSpeak.connected()) {
      Serial.println("Conectado a Thingspeak...");
      Serial.println();
     }else{
      Serial.println("No Conectado a Thingspeak...");
      Serial.println();
     }
   }
}

//************************************************************ PWM del triac ********************************************
//Controlamos cuanto tiempo está encendido el triac dado un periodo T 
// T = el periodo del PWM configurado y  D = dutty en %  
// Ejemplo:  PWM(20000,30) => para un periodo de 20 sg, el triac estaría estar un 30%(6sg) encendido y un 70% apagado (14sg)
//***********************************************************************************************************************
bool PWMtriac(int T,byte D){
  
  t2=millis();
  
  Serial.print(T);Serial.print(" ");Serial.print(D);Serial.print(" ");
  Serial.println(T*D/100);
  Serial.print("tiempo t2-t1: ");
  Serial.println(t2-t1);
  
  if (D==100){                       // Si el Dutty es de 100% directamente significa que lo quiero todo el rato encendido, salvo nueva orden
    t1=millis();
    return Encendido;
  }
  else if( t2-t1 < (T*D/100) ){      // Triac Encendido mientras  t2 no llegué al tiempo de dutty deseado
    Serial.println("A");
    return Encendido;
  }
  else if ( ((t2-t1) >= (T*D/100)) & ((t2-t1) < T ) ) {      // Una vez superado el t. de dutty, el triac debe estar apagado hasta acabar el peridod del PWM configurado
     Serial.println("B");
     return Apagado;
  }else if ( (t2-t1) >= T ) {                            // Una vez superado el t. de periodo del dutty por defecto dejo el triac apagado  y reseteo el valor del tiempo de referencia a medir t1.
     Serial.println("C");
     t1=millis();     // inicio referencia temporal
     return Apagado;
  }
}
//***********************************************************************************************************************
