/*

SLAVE MODBUS RTU 
----------------

    Codigo de ejemplo para esclavo ModBus WK0400.

EQUIPO DE TRABAJO:
------------------
    David Guijarro <david.guijarro@padrepiquer.net>
    Rodolfo García <kwendenarmo@gmail.com>
    Jorge Gómez <syvic@sindormir.net>

DESCRIPCIÓN:
------------
    Este código permite leer los sensores analógicos y digitales accediendo a los registos siguientes:

    REGISTRO    VALOR ALMACENADO
    --------    ----------------
       4            digital 1
       5            digital 2
       6            digital 3
       7            digital 4
       8            digital 5
       9            digital 6
       10           analog Potenciómetro
       11           analog Entrada Analógica Auxiliar

    Para actuar sobre los relés, los registros asociados son los siguientes:

    REGISTRO    RELÉ ASOCIADO
    --------    -------------
       0            Relé R1
       1            Relé R2
       2            Relé R3
       3            Relé R4

NOTAS:
------
    - IMPORTANTE: Este código debe ser compilado desde el entorno Arduino 0023. No funciona bien con Arduino 1.
    - Para la programación es necesario bajar todos los jumpers de configuración a nivel bajo para no interferir con el programador.
    - El DIP Switch 5 debe activarse en los elementos de la red que sean extremos de la misma.

TODO:
-----
    - Implementar sistema de autodirección controlada por el Maestro.
    - ¿Dirección por defecto = 18?

*/

#include <ModbusSlave.h>

//Pines I/O
#define Input1  2
#define Input2  3
#define Input3  4
#define Input4  5
#define Input5  6
#define Input6  7
#define Rele1   8
#define Rele2   9
#define Dir0    10
#define Dir1    11
#define Dir2    12
#define Dir3    13
#define Pot     14
#define Rele3   15
#define Rele4   16
#define AInput  17

#define MB_REGS 12

//Objetos
ModbusSlave mbs;

//Variables globales
int regs[MB_REGS];

void setup() {
  byte yAddress = 0;
  
  //Direccion de pines
  pinMode(Rele1, OUTPUT);
  pinMode(Rele2, OUTPUT);
  pinMode(Rele3, OUTPUT);
  pinMode(Rele4, OUTPUT);
  pinMode(Input1, INPUT);   //Entradas inversas!!
  pinMode(Input2, INPUT);   //Entradas inversas!!
  pinMode(Input3, INPUT);   //Entradas inversas!!
  pinMode(Input4, INPUT);   //Entradas inversas!!
  pinMode(Input5, INPUT);   //Entradas inversas!!
  pinMode(Input6, INPUT);   //Entradas inversas!!
  pinMode(Dir0, INPUT);     //Entradas inversas!!
  pinMode(Dir1, INPUT);     //Entradas inversas!!
  pinMode(Dir2, INPUT);     //Entradas inversas!!
  pinMode(Dir3, INPUT);     //Entradas inversas!!

  //Configuración de dirección
  if (digitalRead(Dir0) == LOW) yAddress += 1;
  if (digitalRead(Dir1) == LOW) yAddress += 2;
  if (digitalRead(Dir2) == LOW) yAddress += 4;
  if (digitalRead(Dir3) == LOW) yAddress += 8;

  mbs.configure(yAddress,9600,'n',0);
}

void loop() {

  mbs.update(regs, MB_REGS);

  //Ejecución en actuadores
  regs[0] != 0 ? digitalWrite(Rele1, HIGH) : digitalWrite(Rele1, LOW);
  regs[1] != 0 ? digitalWrite(Rele2, HIGH) : digitalWrite(Rele2, LOW);
  regs[2] != 0 ? digitalWrite(Rele3, HIGH) : digitalWrite(Rele3, LOW);
  regs[3] != 0 ? digitalWrite(Rele4, HIGH) : digitalWrite(Rele4, LOW);

  //Lectura y envío de ensores
  regs[4] = ~digitalRead(Input1);
  regs[5] = ~digitalRead(Input2);
  regs[6] = ~digitalRead(Input3);
  regs[7] = ~digitalRead(Input4);
  regs[8] = ~digitalRead(Input5);
  regs[9] = ~digitalRead(Input6);
  regs[10] = analogRead(AInput);
  regs[11] = analogRead(Pot);
}
