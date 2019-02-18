
### @JoseFdo_

##  Control Automático Curvas de Temperatura de Horno para Cerámica
Se trata de modernizar un horno de cerámica comprado por Wallapop que disponía de un cuadro eléctrico antiguo para el control de las curvas de temperaturas, dicho control, era totalmente manual y tenía partes dañadas ( temporizadores ), por otra parte, el contactor que usa, produce unos altos niveles de armónicos.

Por lo cual se trata de actualizar el controlador usando:

- NodeMCU ESP-12F 0.96 Pulgadas OLED Digital CP2102 ESP8266 WiFi Placa de Desarrollo inalámbrico para Arduino Wemos. (Esta placa dispone de Wifi y pantalla OLED).
- Control de potencia usando; MOC3041 + triac BTA41 + disipador
- Sensor de temperatura de adafruit MAX31856 para poder medir usando protocolo SPI temperatura del horno ( hasta 1200 º C ) que emplea un termistor de tipo S.

Nos apoyamos en la plataforma "ThingSpeak" para crear un canal sobre el cual enviaremos cada 20sg la temperatura del horno de forma que podamos monitorizar en todo momento la temperatura del horno.

## Agradecimientos  

Agradecer a <b>Adafruit</b> por su librería para el manejo del MAX31856 y a <b>Luis del Valle</b> por su magnífica documentación en <b>programarfacil.com</b> así como a <b>ThingSpeak</b> por su maravillosa plataforma para el iot que nos permite de forma fácil conectar nuestros dispositivos a la nube y realizar un poco de iot.
