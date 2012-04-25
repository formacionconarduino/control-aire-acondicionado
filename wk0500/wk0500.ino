/*

MAESTRO MODBUS-RTU / ESCLAVO MODBUS-TCP
---------------------------------------
    Codigo de ejemplo para maestro ModBus WK0500 en RTU y esclavo en TCP.

EQUIPO DE TRABAJO:
------------------
    David Guijarro <david.guijarro@padrepiquer.net>
    Rodolfo García <kwendenarmo@gmail.com>
    Jorge Gómez <syvic@sindormir.net>

DESCRIPCIÓN:
------------


NOTAS:
------
    - 

TODO:
-----
    - Hacer tabla de correspondencia entre numeros de esclavo reales e indice en el array 
    - Limpiar un poco el codigo

*/

 
//Librerias
#include <ModbusMaster.h>  //Maestro Modbus RTU
#include <Mudbus.h>        //Esclavo Modbus TCP
#include <SPI.h>           //Comunicacion wiznet
#include <Ethernet.h>      //Ethernet
#include <Server.h>        //Servidor TCP/IP

//Defines
#define VERSION "v0.1.3"
#define DEBUG 1
#define NUM_SLAVES 4
#define LEDY 60
#define LEDR 61

//Variables globales
byte mac[] = { 0xCA, 0xFE, 0xCA, 0xFE, 0xCA, 0xFE};
byte ip[] = { 10, 0, 1, 254};
byte subnet[] = { 255, 255, 255, 0 };
byte gateway[] = { 10, 0, 1, 1 };
char buffer_entrada[1024];

//Objetos
EthernetServer server(80);          //Servidor web
ModbusMaster slaves[NUM_SLAVES];    //Declaración de esclavos
ModbusMaster termostato;            //El termostato esta en una direccion no correlativa
Mudbus Mb;                          //Modbus TCP Slave


void setup() {

  for (int i=0; i<NUM_SLAVES; i++){ //Inicializacion de esclavos
    slaves[i].setSlave(i+1);
    slaves[i].setUSART(1);
    slaves[i].begin(9600);
  }
  
  //Inicializacion del esclavo termostato
  termostato.setSlave(254);
  termostato.setUSART(1);
  termostato.begin(9600);
  

  if (DEBUG) Serial.begin(9600);  //Puerto serie para debug

  Ethernet.begin(mac, ip);

  server.begin();
  if (DEBUG) Serial.println("Listening...");

  delay(1000);
}

void loop() {
  unsigned int i;
  byte c;
  EthernetClient client;

  byte last_slave=0, actual_slave;
  unsigned int last_relay=0, last_value=0;
  unsigned int actual_relay, actual_value; 
  
  while(1) {
    
    Mb.Run(); //Inicio de Mosbus slave TCP
    
    //Lectura de entradas analogicas (Simulado)
    Mb.R[0] = 123;
    Mb.R[1] = 124;
    Mb.R[2] = 125;
    Mb.R[3] = 126;
    Mb.R[4] = 127;
    Mb.R[5] = 128;
    
    actual_slave = Mb.R[10];
    actual_relay = Mb.R[11];
    actual_value = Mb.R[12];
    
    if ((last_slave != actual_slave) || (last_relay != actual_relay) || (last_value != actual_value)) {
      Write485(actual_slave, actual_relay, actual_value);
      last_slave = actual_slave;
      last_relay = actual_relay;
      last_value = actual_value;
    }
    
    //Analog outputs 0-255
    //analogWrite(6, Mb.R[6]); //pin ~6 from Mb.R[6]
  
    //Digital outputs
    //digitalWrite(8, Mb.C[8]); //pin 8 from Mb.C[8]
  
    client= server.available();
  
    if (client == true) {
      boolean currentLineIsBlank = true; // an http request ends with a blank line
      i=0;
  
      while (client.connected()) {
        if (client.available()) {
          c = client.read();
          if (DEBUG) Serial.print("!");
  
          buffer_entrada[i++]=c;
          if (i >= 1024) i=0;  //Proteccion contra buffer overflow
  
          //COMIENZO SERVIDOR HTTP
          if (c == '\n' && currentLineIsBlank) {
            do_serve_web(client);
            buffer_entrada[i++]='\0';
            if (DEBUG) Serial.println(buffer_entrada);
            client.stop();
            break;
          }
          if (c == '\n') currentLineIsBlank = true; // you're starting a new line
          else if (c != '\r') currentLineIsBlank = false; // you've gotten a character on the current line
          //FIN SERVIDOR HTTP
        }
      }
      delay(1);
      client.stop();
    }
  
    //Parseo de los comandos recibidos via HTTP
    if (strlen(buffer_entrada)>0) {
      if (DEBUG) Serial.println("Mandando al parser: ");
      if (DEBUG) Serial.println(buffer_entrada);
      if (DEBUG) Serial.println("");
      parser(buffer_entrada);
      buffer_entrada[0]='\0';
    }
  }

}

void do_serve_web(EthernetClient client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println();
  client.print("<html><head><title>Modbus webserver controller ");
  client.print(VERSION);
  client.println("</title></head><body>");
  
  for (int i=0; i<NUM_SLAVES; i++) {
    client.print("<p>");
    for (int j=0; j<4; j++)
      for (int k=0; k<2; k++)
        imprime_html(client, i,j,k);
  }
  client.println("</body></html>");
}

byte parser(char* pstrInput)
{
  unsigned int iLoop=0;
  byte yState=0;
  byte ySlave=0;
  byte yRelay=0;
  unsigned int iValue=0;
  byte yOffset=0;
  char strBuffer[6];
  char strKey;

  do {
    //Recorremos el array hasta encontrar un '?' o un '&' hasta llegar al final de la cadena
    while (((pstrInput[iLoop] != '?')  && (pstrInput[iLoop] != '&')) && pstrInput[iLoop] != '\0') iLoop++;
    iLoop++;  //avanzamos al caracter siguiente
    strKey=pstrInput[iLoop];  //guarda el caracter clave

    yOffset=0;
    iLoop += 2;  //Saltamos el igual
    
    do {
      strBuffer[yOffset++] = pstrInput[iLoop++];
    } while ((pstrInput[iLoop] <= '9') && (pstrInput[iLoop] >= '0'));
    
    strBuffer[yOffset] = '\0';

    switch(strKey) {
      case 's': //Tratamiento del valor de esclavo
        ySlave = atoi(strBuffer);
        yState++;
        break;
      case 'r': //Tratamiento del valor de rele
        yRelay = atoi(strBuffer);
        yState++;
        break;
      case 'v': //Tratamiento del valor de valor
        iValue = atoi(strBuffer);
        yState++;
        if (iValue==9) barrido();
        break;
    }
  } while (((pstrInput[iLoop] != '\0') && (pstrInput[iLoop] != '\n')) && (yState < 3)); //Parsea hasta un retorno de carro, final del buffer o hasta que parsee las tres entradas.

  if (yState == 3) {
    Write485(ySlave, yRelay, iValue);
    return 1;
  }
  else return 0;
}

//Realiza la escritura de un rele en un esclavo
void Write485(byte slave, unsigned int relay, unsigned int value) {
  
  if (DEBUG) Serial.println("Escribiendo al esclavo: ");
  if (DEBUG) Serial.println(slave, DEC);
  if (DEBUG) Serial.println(relay, DEC);
  if (DEBUG) Serial.println(value, DEC);
  if (slave==4)
    termostato.writeSingleRegister(relay,value);
  else
    slaves[slave].writeSingleRegister(relay,value);
  if (DEBUG) Serial.println();
}

//Funcion que realiza un barrido por todos los reles de todos los eslavos
void barrido(void) {
  for (int i=0; i<NUM_SLAVES; i++)
    for (int j=0; j<4; j++)
      for (int k=0; k<2; k++){
        Write485(i,j,k);
        delay(300);
      }
}

void imprime_html(EthernetClient client, byte s, byte r, byte v)
{
  client.println("<a href=\"cgi.cgi?s=");
  client.print(s);
  client.print("&r=");
  client.print(r);
  client.print("&v=");
  client.print(v);
  client.print("\">");

  client.print(" S");
  client.print(s);
  client.print(" R");
  client.print(r);
  client.print(" V");
  client.print(v);
  client.print("</a><br>");
}

