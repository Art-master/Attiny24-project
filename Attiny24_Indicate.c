/*******************************************************
This program was created by the CodeWizardAVR V3.24
Automatic Program Generator
© Copyright 1998-2015 Pavel Haiduc, HP InfoTech s.r.l.
http://www.hpinfotech.com

Project :
Version :
Date    : 19/11/2015
Author  :
Company :
Comments:


Chip type               : ATtiny24
AVR Core Clock frequency: 8.000000 MHz
Memory model            : Tiny
External RAM size       : 0
Data Stack size         : 32
*******************************************************/
//Подключаем необходимые библиотеки
#include <io.h>
#include <delay.h>
#include <tiny24.h>
#include <interrupt.h>

#define DURATION 100 /*Длительность 1 импульса*/
#define CRC_POLINOME 4 /*Полином для расчета CRC*/
#define FT_SPEED_SENSOR 6000 //максимальная частота датчика колебания/мин (100 в секунду)
#define MAX_SPEED 120
#define MAX_LENGHT_SEND_WORD 24
#define START_BYTE 0xFF

// объявляем необходимые функции
void send_word(int);   //Отправка слова по каналу.
void speed_parse(int); //Разбор данных с датчика скорости и перевод в нужный формат.
void TIMER_16bit_Init(void); //Метод, инициализирующий шестнадцатибитный таймер.
void TIMER_8bit_Init(void); //Метод, инициализирующий восьмибитный таймер.
void speed_check(void); //Опрос датчика скорости
void sendPackage(int);

//Сигнал состояния текущего отправляемого бита
unsigned int state=0;
//Отравляемые пакет побитно,
// где каждый элемент массива является битом двоичной системы счисления.
unsigned int arrayChars[MAX_LENGHT_SEND_WORD];

unsigned char flagSpeed= 1; //Флаг контроля скорости
unsigned int clockSpeed= 0; //Частота датчика скорости.
unsigned int currentSpeed=0; //текущая скорость автомобиля

//Скорость автомобиля за один километр. Вычисляется как отношение частоты
//датчика и максимальной скорости развиваемой автомобилем.
unsigned int speed1km=FT_SPEED_SENSOR/MAX_SPEED;

unsigned int firstLoad=0;// Флаг первичного запуска алгоритма

void main(void)
{
//unsigned int openPort=0;

DDRB=0b00001011; /* порты 1 и 2 PB0 на выход */
DDRA=0b01000000; /* порты 1-5 и 7 PA0 на вход */
PORTB = 0b00000000;
PORTA = 0b10111111;//Подключаем подтягивающие PULL-UP резисторы ко всем пинам порта A.
PORTA.6=1; //разрешающий сигнал для оптопары

TIMER_8bit_Init();// инициализируем 8-ми разрядный таймер-счетчик
TIMER_16bit_Init(); // инициализируем 16-и разрядный таймер-счетчик

//прерывания
sei(); //Глобальное разрешение прерываний.
MCUCR =(1<<ISC01)|(1<<ISC00);//Срабатывание по переднему фронту.
GIMSK = (1<<INT0); //Настраиваем прерывания от внешнего источника.

/*
 Для начала сделаем небольшой участок кода,
  который будет опрашивать датчики,
   ответственные за положение рукоятки КПП автомобиля.
*/
   while(1)
   {
     if (PINA.0==0) {        //1 скорость
            sendPackage(1);
      }else if (PINA.1==0) { //2 скорость
            sendPackage(2);
      }else if (PINA.2==0) { //3 скорость
            sendPackage(3);
      }else if (PINA.3==0) { //4 скорость
            sendPackage(4);
      }else if (PINA.4==0) { //5 скорость
            sendPackage(5);
      }else if (PINA.5==0) { //6 скорость
            sendPackage(6);
      }else{
            sendPackage(7);  //задняя
      };

      /*подсчитываем колличество тактов и рассчитываем
      скорость движения
      */
      if(flagSpeed & TCNT0>=234){//30 ms  //пресмотреть !!!!!!!
        speed_parse(clockSpeed);
        //TCNT0=0;
      };

       /*Если по прошествии некоторого времени есть информация о
      тактах счетчика скорости, то разрешаем его дальнейшее использование,
      иначе запрещаем
      */
      if(TCNT0>=230){
         if(clockSpeed>0){
            flagSpeed=1;
        }else{
            flagSpeed=0;
        };
     };

     //при подаче питания сигнал о вкл пер.
     if(firstLoad==0){
        speed_check();
        firstLoad=1;
      };
   };
};

 /* при подаче питания и если не вкл. нейтр.
 создает характерный писк
 */
void speed_check()
{
    if(PINA!=0){
       PORTB.0 = 1;
       delay_ms(100);
       PORTB.0 = 0;
    };
};

 /* Инициализация 8-разрядного таймера-счетчика
    -все биты WGM02…WGM00 равны нулю (режим Normal)
    -предделитель 1024 (биты CS02…CS00)
 */
void TIMER_8bit_Init(void)
{
    TCCR0A = 0;
    TCCR0A = (0<<WGM02)|(0<<WGM01)|(0<<WGM00);
    TCCR0B = 0;
    TCCR0B = (0<<CS02)|(0<<CS01)|(0<<CS00); // stop/ делитель
    TIMSK0= (0 << TOIE0)|(1 << OCIE0A);
    OCR0A=5;

    TCNT0=0;
}

 /* Инициализация 16-разрядного таймера-счетчика
    -все биты WGM13 WGM12 равны нулю (режим Normal)
    -предделитель 1 (биты CS12…CS10)
    -ISC01 и ISC00 на передний фронт импульса
 */
void TIMER_16bit_Init(void)
{
    if(flagSpeed){
        TCCR1A=(0<<WGM13)|(0<<WGM12)|(0<<WGM11)|(0<<WGM10);
        TCCR1B=(0<<CS12)|(0<<CS11)|(1<<CS10);
        TCNT1 = 0;
    }else{
        TCCR1B=(0<<CS12)|(0<<CS11)|(0<<CS10);
    };
}

void sendPackage(int speedSensor){
    if(state==0){   //ждем пока отправится предыдущее слово
         int word =0;
         word=START_BYTE;
         word|=currentSpeed<<8;
         word|=speedSensor<<16;
         word|=((START_BYTE+currentSpeed+speedSensor)%CRC_POLINOME)<<16; //CRC
         send_word(word);
    };


}
 /* отправка слова по каналу  */

 void send_word(int word){
     int i=0;
     state=1;
     for(i=0; i<=MAX_LENGHT_SEND_WORD; i++){
       if((word & (1<<i))==(1<<i)){
          arrayChars[i]=1;
          word^=(1<<i);
       }else{
          arrayChars[i]=0;
       };
       if(word==0){
        break;
       }
    };
    TCCR0B = (0<<CS02)|(0<<CS01)|(1<<CS00);
};

//внутреннее прерывание. от таймера.
ISR(TIM0_COMPA)
{
PORTB.1=arrayChars[state];
arrayChars[state]=0;
state++;
TCNT0=0;
      if(MAX_LENGHT_SEND_WORD==(state-1)){
         state=0;
         TCCR0B = (0<<CS02)|(0<<CS01)|(0<<CS00);
      };
};

//высчитывает текущую скорость
 void speed_parse(int speedValue){
     int ft_sec=(((speedValue/60)/1000)*30);//колл-во тактов за 30 мс.
     currentSpeed=ft_sec*speed1km; //отсчитывается текущай ск.
     if(currentSpeed>255){
      currentSpeed=255;
     };
};

//внешнее прерывание по входу. обработчик.
ISR(EXT_INT0)
{
   if(flagSpeed){
       clockSpeed++;
   };
};