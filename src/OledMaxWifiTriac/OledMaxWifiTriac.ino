
// *****************  Status:OK probado con Placa NodeMCU 1.0 ESP12E  ***********************************++++

// Incluir la librería ESP8266WiFi **********************************
#include <ESP8266WiFi.h>

//*******************************************  Wifi conexión ESP a Router **********************************************
IPAddress ip,gateway,mask; 
char ssid[] = "vodafone659F_up"; // También valdría string ssid[] = "nombre_wifi";
char pass[] = "Q9TW8G6VWZGJPP"; // También valdría string pass[] = "contraseña_wifi";
boolean conectado=false;
//**********************************************************************************************************************


//********************************************* Datos thingSpeak  y Cte Wifi *******************************************

WiFiClient clienteThingSpeak;   // Objeto Cliente de ThingSpeak
boolean ThingSpeakON;

char direccionThingSpeak[] = "api.thingspeak.com";
String APIKey = "3PJ6HLYPMYU0MHUP";
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

//int Temp[] = {100,120,150,100};
//int tiempo[]= {30,30,30,30};

//***********  0  1   2   3   4   5   6   7   8   9  10  11  12  13  14  15  16   17  18  19  20  ************************************
int Temp[] = {80,110,130,160,190,240,290,340,410,460,510,560,610,660,760,860,1025,900,800,750,700};   // [ 0 a 20 ]   
int tiempo[]= {40,40, 40, 40, 40, 40, 40, 40, 40, 40,40, 40, 40, 40, 40, 40, 25,  30, 30, 30, 30 };

int nTemp=sizeof(Temp)/4;             //Devuelve el número de bytes ( como en el ESP8266  un entero ocupa 4 bytes --> lo divido por 4 )
int nTiempo=sizeof(tiempo)/4;

int Tpwm=20000;   // Periodo del PWM 20 sg.  Al ser byte ( 255 como máximo )
float dutty=100;
float t1=0,t2=0;
bool estadoPWM=false;

// PID
float SumErr=0;
float kp=1.2;  float ki=0.3;  float kd=1.8;    // Kp se irá modificando conforme se aumente la temperatura objetivo ya que cada vez le costará más conseguir la temperatura objetivo
float PID_p=0; float PID_i=0; float PID_d=0;
float target=Temp[0];                                       // Tª objetivo
float PID_errorActual=0,PID_errorAnterior=0;                // Error Anterior y Actual respecto al valor objetivo marcado
float PID_valor=0;
float t1_pid=0,t2_pid=0;                 //muestreo pid      marcas de tiempo para controlar el muestreo PID cada TmuestreoPID ms
float TmuestreoPID=100;                 // Periodo de muestreo del PID
//************************************************************************************************************************


//***************** Control de "Etapa" en el que se encuentra **********************************************************

byte curvaNum=0;                     // Empezamos por la etapa 1
bool inicioCurva=false;           // Para marcar cuando una etapa a empezado  y  empezar a cronometrar pues el tiempo que debo permanecer en dicha etapa
float t1_Curva=0, t2_Curva=0;       // marcas de tiempo para controlar el tiempo que permanezco en dicha etapa
float T_Curva=tiempo[0]*60*1000;          // Permanecer 30 minutos en cada etapa.
bool  fin=false;
bool  faseDeSubida=true;
//*********************************************************************************************************************


void setup() {

  Serial.begin(115200);
    
  pinMode(triac,OUTPUT);              // Configuración pin de activación del triac.  El Triac lo activamos por el pin D4
  digitalWrite(triac,Apagado);

  maxthermo.begin();                                         //************ Max *************************                
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

   int intentos=0;
   
   display.setFont(ArialMT_Plain_10);
   WiFi.begin(ssid, pass);
   
 
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

  display.setFont(ArialMT_Plain_16);

  float tempInicial=maxthermo.readThermocoupleTemperature()+ offset;
  curvaNum = AveriguaraEnQueCurvaHaArrancado(tempInicial);
  
} 

//********************************************************************** PROGRAMA PPAL ****************************************************************************************

void loop() {
  
  target=Temp[curvaNum];                                           //la temperatura objetivo varia en función de "curvaNum"  me encuentre
  T_Curva=tiempo[curvaNum]*60*1000;                                //Tiempo que debo permanecer en esa curva una vez alcanzado el tiempo target por primera vez
  
  float  valor=maxthermo.readThermocoupleTemperature()+ offset;    // Tª del horno
  String temp= String(valor);

  kp = revisarKp(valor);
  
  // Datos para thingSpeak *******************************
  
  String datosThingSpeak = "field1=" + temp;

  tiempoActual=millis();                         //Muestreo de ThingSpeak
  if(tiempoActual-tiempoAnterior >= muestreo){   //Si han pasado los 20 sg para poder enviar a ThingSpeak
    tiempoAnterior=millis();
    EnviarDatosThingSpeak(datosThingSpeak);        //Envia y revisaremos si han pasado 20 sg con la función millis
  }

    dutty = CalculoPID(target,valor);        // Calcula el PID para el target deseado
    bool estadoPWM = PWMtriac(Tpwm,dutty);   // para el dutty calculado con el PID encenderé más o menos tiempo el triac para conseguir la temp deseada.
   
    if(fin==false)
      digitalWrite(triac,estadoPWM);           // actuo sobre el triac
      else
        digitalWrite(triac,Apagado);           //Cuando llegue al final de todas las curvas --> Apago el triac y dejo simplemente enfriar

    if(curvaNum <= PuntoInflexion()){
    curvaNum=EnQueCurvaEstoySubiendo(valor,target);  // compruebo en que curva de SUBIDA estoy
    faseDeSubida=true;
    }
      else{
        curvaNum=EnQueCurvaEstoyBajando(valor,target);  // compruebo en que curva de BAJADA estoy 
        faseDeSubida=false;
      }
        
    if (curvaNum >= (nTemp-1) ) fin=true;
    else fin=false;

    MostrarTemp(temp);
    
   //********************************************************************************************************************************************************************
   // Nota: He apreciado que con un PWM 5% sobre T=20sg  ( 1sg) la temperatura se llega a estabilizar y al final incluso empieza a subir un poco si mantenemos ese dutty
   // así durante media hora  ( tengo mi media hora estable para cambiar a otro punto de temperatura )
   //*******************************************************************************************************************************************************************
}

//*******************************************************************************************************************************************************************************************************************************
//*************************************************************************************** Funciones Adicionales ***************************************************************************************************************************************************
//*****************************************************************************************************************************************************************************************************************************


//*********  Con esta función recalculamos la constante kp ( aumenta al aumentar la temperatura target ) .********************************************
//****** para poder alcanzar temperaturas más altas, necesitaremos mayor kp *************************************************************************
float revisarKp(float t){

float newKp;
   if ( t <= 250 ) newKp=kp;
   else if ( t > 250 && t <= 500) newKp=2.5;
   else if ( t > 500 ) newKp=7;

   return newKp;
}


//********************************* Calculamos el punto de inflexión de la matriz de temperaturas  ( donde cambia de curva de subida a curva de bajada ( enfriamiento )  ) ******************
//******************************************************************************************************************************************************************************************
int PuntoInflexion(){

int p=0;

  for (int i=0; i<(nTemp-1); i++){
    if(Temp[i+1] > Temp[i]) p=i;
    else {
       p=i;
       break;
    }
  }
  return p;
  Serial.print("El punto de inflexión que define el cambio ETAPA DE SUBIDA a ETAPA DE BAJADA está en la curva: "); Serial.println(p);
}


//************************************************************************* Examina la curva de Subida *************************************************************************************
// Examina las partes de la Curva de temperatura en la que se encuentra para saber cuanto tiempo debe permanecer dentro de esa curva y cuando decidir cambiar a la siguiente CURVA DE SUBIDA
//******************************************************************************************************************************************************************************************

byte EnQueCurvaEstoySubiendo(float temperatura,float target){                         // PROBLEMA: QUE PASA SI NO LLEGA AL TARGET --> NUNCA ARRANCARÍA LA CUENTA --> COMO LO CONTROLO ?

float tiempo;

faseDeSubida=true;

if(fin==false){

  tiempo=(t2_Curva-t1_Curva);  //De inicio las dos son iguales a 0.

  if( (inicioCurva==false) && (temperatura < target-30)) {                                                                      //etapa 1ª de la Curva
    inicioCurva=false;
    t1_Curva=millis(); 
    t2_Curva=millis();
  }
  if ( (temperatura >= target-30) && (inicioCurva==false ) ) {  //solo la primera vez  que se supera la temperatura objetivo    //etapa 2ª  --> Punto inicio de Cuenta
    inicioCurva=true;
    t1_Curva=millis();                                      //En el punto de partida los dos tiempos son iguales
    t2_Curva=millis();
  }

  if ( (inicioCurva) && (tiempo <=T_Curva))  {     // Si he alcanzado la temperatura                                        // etapa 3ª  Me mantengo en la curva un tiempo T_Curva
    t2_Curva=millis();
    inicioCurva=true;
  }
  if ( (inicioCurva) && (tiempo > T_Curva) ){
    inicioCurva=false;
    curvaNum=curvaNum+1;
    t1_Curva=millis(); 
    t2_Curva=millis();
  }
}
    return curvaNum;   
}

//************************************************************************* Examina la curva de BAJADA *************************************************************************************
// Examina las partes de la Curva de temperatura en la que se encuentra para saber cuanto tiempo debe permanecer dentro de esa curva y cuando decidir cambiar a la siguiente CURVA DE BAJADA
//******************************************************************************************************************************************************************************************
byte EnQueCurvaEstoyBajando(float temperatura,float target){

float tiempo;

faseDeSubida=false;

if (fin==false){
 
  tiempo=(t2_Curva-t1_Curva);  //De inicio las dos son iguales a 0.

  if( (inicioCurva==false) && (temperatura > target+5)) {                                                                      //etapa 1ª de la Curva
    inicioCurva=false;
    t1_Curva=millis(); 
    t2_Curva=millis();
  }
  if ( (temperatura <= target+5 ) && (inicioCurva==false ) ) {  //solo la primera vez  que se supera la temperatura objetivo    //etapa 2ª  --> Punto inicio de Cuenta
    inicioCurva=true;
    t1_Curva=millis();                                      //En el punto de partida los dos tiempos son iguales
    t2_Curva=millis();
  }

  if ( (inicioCurva) && (tiempo <=T_Curva))  {     // Si he alcanzado la temperatura                                        // etapa 3ª  Me mantengo en la curva un tiempo T_Curva
    t2_Curva=millis();
    inicioCurva=true;
  }
  if ( (inicioCurva) && (tiempo > T_Curva) ){
    inicioCurva=false;
    curvaNum=curvaNum+1;
    t1_Curva=millis(); 
    t2_Curva=millis();
  }
}
    return curvaNum;   
}



//***********************************************************************************************************************************************************
// Mostrar en la pantalla OLED la Temperatura en tiempo real, y  2 marcas para saber si tengo conexión con el router y conexión con ThingSpeak ***************
//************************************************************************************************************************************************************
void MostrarTemp(String temp){
  
  display.clear();
  display.drawString(10, 2 , "Temperatura: ");
  display.drawString(10, 25,temp);
  if(conectado){
    display.drawString(90,40,"O");
    if(ThingSpeakON) display.drawString(110,40,"O");      // Pongo marca en pantalla para saber si tengo conexión con ThingSpeak
      else display.drawString(110,40,"X");
  }else{
    display.drawString(90,40,"X");
    if(ThingSpeakON) display.drawString(110,40,"O");
      else display.drawString(110,40,"X");
  }
  display.drawString(10,45,String(curvaNum));     //Muéstrame en qué curva estoy  y si la curva ya está en su fase de estabilidad ( contando el tiempo que debe permanecer en esa curva )
  display.drawString(40,45,String(inicioCurva));

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
      Serial.println("......");
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
bool PWMtriac(int T,float D){          
  
  t2=millis();
  
  if (D==100){                       // Si el Dutty es de 100% directamente significa que lo quiero todo el rato encendido, salvo nueva orden
    t1=millis();
    return Encendido;
  }
  else if( t2-t1 < (T*D/100) ){      // Triac Encendido mientras  t2 no llegué al tiempo de dutty deseado
    return Encendido;
  }
  else if ( ((t2-t1) >= (T*D/100)) & ((t2-t1) < T ) ) {      // Una vez superado el t. de dutty, el triac debe estar apagado hasta acabar el peridod del PWM configurado
    return Apagado;
  }else if ( (t2-t1) >= T ) {                            // Una vez superado el t. de periodo del dutty por defecto dejo el triac apagado  y reseteo el valor del tiempo de referencia a medir t1.
     t1=millis();     // inicio referencia temporal
     return Apagado;
  }
}

// ******************* Calculo del valor PID  ( Actua sobre el Dutty del PWM --> Cuanto tiempo debe estar encendido el triac ) ************************

byte CalculoPID(float target,float valor){

  
  t2_pid=millis();

  //if(faseDeSubida==true){
  //  target=target;                             //Para el PID le fuerzo como 10 grados por encima ( solo cuando está en la etapa de subida no cuando esté en la etapa de bajada (enfriando ) )
  //}else target=target;
  
  if( (t2_pid-t1_pid) >= TmuestreoPID){          //Solo recalculo el PID cada cierto tiempo ( Periodo de muestreo )

    t1_pid=millis();             //reinicio cronómetro cuenta del muestreo
    
    PID_errorActual=target-valor;      //Cálculo del ERROR
    PID_p=kp*PID_errorActual;          // P

    //Ojo con la Integral. Si tiempo de muestreo es mucho mas corto que lo que tarda el sistema en poder reaccionar --> puede rápidamente acumular un valor muy elevado y disparar ( llevar al traste todo el control)
    //if( PID_errorActual > -2  &  PID_errorActual <2 ){        // Si el error de temperatura está + - 3 grados --> aplico parte integral
    //SumErr=SumErr+(PID_errorActual*TmuestreoPID);
    //PID_i=ki*SumErr;
    //}else PID_i=0;  // Solo quiero que actue  en un rango muy pequeño de temp ( corregir oscilaciones ?? )
       
    PID_d=kd*1000*(PID_errorActual-PID_errorAnterior)/(TmuestreoPID);  // D
    
    PID_valor=PID_p+PID_d+PID_i;

    if (PID_valor < 0) {           //limites Inferior/Superior
    PID_valor=0;
    }else if (PID_valor > 100){
    PID_valor=100;                 //limito a  un máximo del 90% de la carga. Como mucho entregaré el 90% 
    }

     PID_errorAnterior=PID_errorActual;
     
    Serial.print("Ha terminado todas las curvas? "); if(fin) Serial.println(" Siiii"); else Serial.println("Nooo");
    Serial.print("Estoy en la Curva: "); Serial.print(curvaNum); Serial.print(" de un total de "); Serial.println(nTemp-1);
    Serial.print("Punto de inflexión en la curva: "); Serial.println(PuntoInflexion());
    Serial.print("Cuenta dentro de la Curva actual iniciada ? "); if(inicioCurva) Serial.println("Si"); else Serial.println("No");
    
    Serial.print("Error Actual: ");Serial.println(PID_errorActual);
    Serial.print("Temperatura Objetivo de esta Curva es: "); Serial.println(target);
    Serial.print("Temperatura Actual: "); Serial.println(valor);
    Serial.print("kp= ");Serial.println(kp);
    Serial.print("P: "); Serial.println(PID_p); Serial.print("I: "); Serial.println(PID_i);Serial.print("D: "); Serial.println(PID_d);
    Serial.print("PID: "); Serial.println(PID_valor);
    Serial.print("Dutty Applicado "); Serial.println(dutty);
    Serial.println("..............................");
        
   
    
   }else{
    t1_pid=t1_pid;  // No hago nada
  }

   return PID_valor;
}

// ******************************* Averiguar en que curva Estoy al arrancar el progrma. **************************************************************************
// *************** Esto será útil por si se cuelga algo, se va la luz y así, si se reincia, sigua la curva en función de a que temperatura se encuentre **********
// ***************  ¿ Pero como saber si estaba en curva de subida o bajada ? --> tendría que guardar esa información en EEPROM para que no se borrase al iniciar
// ***************   y  en función del valor de EEPROM y la temperatura que me encuentre , saber en que curvaNum me encuentro realmente
//  **************************************************************************************************************************************************************

byte AveriguaraEnQueCurvaHaArrancado(float t){

  //Debo leer de la EEPROM una variable para saber si estaba en la fase de SUBIDA o la de BAJADA
  //Debo medir la temperatura a la que me encuentro y así mirando el array con todas las temperaturas que tengo decidir en que curva me encuentro
  
  return curvaNum;
}
