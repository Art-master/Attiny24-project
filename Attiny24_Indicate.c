/*******************************************************
This program was created by the CodeWizardAVR V3.24
Automatic Program Generator
� Copyright 1998-2015 Pavel Haiduc, HP InfoTech s.r.l.
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
//���������� ����������� ����������
#include <io.h>
#include <delay.h>
#include <tiny24.h>
#include <interrupt.h>

#define DURATION 100 /*������������ 1 ��������*/
#define CRC_POLINOME 4 /*������� ��� ������� CRC*/
#define FT_SPEED_SENSOR 6000 //������������ ������� ������� ���������/��� (100 � �������)
#define MAX_SPEED 120
#define MAX_LENGHT_SEND_WORD 24
#define START_BYTE 0xFF

// ��������� ����������� �������
void send_word(int);   //�������� ����� �� ������.
void speed_parse(int); //������ ������ � ������� �������� � ������� � ������ ������.
void TIMER_16bit_Init(void); //�����, ���������������� ����������������� ������.
void TIMER_8bit_Init(void); //�����, ���������������� ������������ ������.
void speed_check(void); //����� ������� ��������
void sendPackage(int);

//������ ��������� �������� ������������� ����
unsigned int state=0;
//����������� ����� �������,
// ��� ������ ������� ������� �������� ����� �������� ������� ���������.
unsigned int arrayChars[MAX_LENGHT_SEND_WORD];

unsigned char flagSpeed= 1; //���� �������� ��������
unsigned int clockSpeed= 0; //������� ������� ��������.
unsigned int currentSpeed=0; //������� �������� ����������

//�������� ���������� �� ���� ��������. ����������� ��� ��������� �������
//������� � ������������ �������� ����������� �����������.
unsigned int speed1km=FT_SPEED_SENSOR/MAX_SPEED;

unsigned int firstLoad=0;// ���� ���������� ������� ���������

void main(void)
{
//unsigned int openPort=0;

DDRB=0b00001011; /* ����� 1 � 2 PB0 �� ����� */
DDRA=0b01000000; /* ����� 1-5 � 7 PA0 �� ���� */
PORTB = 0b00000000;
PORTA = 0b10111111;//���������� ������������� PULL-UP ��������� �� ���� ����� ����� A.
PORTA.6=1; //����������� ������ ��� ��������

TIMER_8bit_Init();// �������������� 8-�� ��������� ������-�������
TIMER_16bit_Init(); // �������������� 16-� ��������� ������-�������

//����������
sei(); //���������� ���������� ����������.
MCUCR =(1<<ISC01)|(1<<ISC00);//������������ �� ��������� ������.
GIMSK = (1<<INT0); //����������� ���������� �� �������� ���������.

/*
 ��� ������ ������� ��������� ������� ����,
  ������� ����� ���������� �������,
   ������������� �� ��������� �������� ��� ����������.
*/
   while(1)
   {
     if (PINA.0==0) {        //1 ��������
            sendPackage(1);
      }else if (PINA.1==0) { //2 ��������
            sendPackage(2);
      }else if (PINA.2==0) { //3 ��������
            sendPackage(3);
      }else if (PINA.3==0) { //4 ��������
            sendPackage(4);
      }else if (PINA.4==0) { //5 ��������
            sendPackage(5);
      }else if (PINA.5==0) { //6 ��������
            sendPackage(6);
      }else{
            sendPackage(7);  //������
      };

      /*������������ ����������� ������ � ������������
      �������� ��������
      */
      if(flagSpeed & TCNT0>=234){//30 ms  //����������� !!!!!!!
        speed_parse(clockSpeed);
        //TCNT0=0;
      };

       /*���� �� ���������� ���������� ������� ���� ���������� �
      ������ �������� ��������, �� ��������� ��� ���������� �������������,
      ����� ���������
      */
      if(TCNT0>=230){
         if(clockSpeed>0){
            flagSpeed=1;
        }else{
            flagSpeed=0;
        };
     };

     //��� ������ ������� ������ � ��� ���.
     if(firstLoad==0){
        speed_check();
        firstLoad=1;
      };
   };
};

 /* ��� ������ ������� � ���� �� ���. �����.
 ������� ����������� ����
 */
void speed_check()
{
    if(PINA!=0){
       PORTB.0 = 1;
       delay_ms(100);
       PORTB.0 = 0;
    };
};

 /* ������������� 8-���������� �������-��������
    -��� ���� WGM02�WGM00 ����� ���� (����� Normal)
    -������������ 1024 (���� CS02�CS00)
 */
void TIMER_8bit_Init(void)
{
    TCCR0A = 0;
    TCCR0A = (0<<WGM02)|(0<<WGM01)|(0<<WGM00);
    TCCR0B = 0;
    TCCR0B = (0<<CS02)|(0<<CS01)|(0<<CS00); // stop/ ��������
    TIMSK0= (0 << TOIE0)|(1 << OCIE0A);
    OCR0A=5;

    TCNT0=0;
}

 /* ������������� 16-���������� �������-��������
    -��� ���� WGM13 WGM12 ����� ���� (����� Normal)
    -������������ 1 (���� CS12�CS10)
    -ISC01 � ISC00 �� �������� ����� ��������
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
    if(state==0){   //���� ���� ���������� ���������� �����
         int word =0;
         word=START_BYTE;
         word|=currentSpeed<<8;
         word|=speedSensor<<16;
         word|=((START_BYTE+currentSpeed+speedSensor)%CRC_POLINOME)<<16; //CRC
         send_word(word);
    };


}
 /* �������� ����� �� ������  */

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

//���������� ����������. �� �������.
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

//����������� ������� ��������
 void speed_parse(int speedValue){
     int ft_sec=(((speedValue/60)/1000)*30);//����-�� ������ �� 30 ��.
     currentSpeed=ft_sec*speed1km; //������������� ������� ��.
     if(currentSpeed>255){
      currentSpeed=255;
     };
};

//������� ���������� �� �����. ����������.
ISR(EXT_INT0)
{
   if(flagSpeed){
       clockSpeed++;
   };
};