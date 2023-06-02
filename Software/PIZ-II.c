#include <18f1220.h>
#device ADC=10
#fuses HS,NOWDT,NOMCLR,PROTECT
#use delay(internal=4M)
#define LCD_DATA_PORT GETENV("SFR:PORTB")
#define LCD_RS_PIN PIN_B0
#define LCD_RW_PIN PIN_B1
#define LCD_ENABLE_PIN PIN_A3
#include <lcd.c>

#bit Buzzer     = 0xF80.2
#bit BInicio    = 0xF80.4
#bit BZapper    = 0xF80.5
#bit Rele       = 0xF81.2

int1  SegPar,Zapper,PWM,Operando;
int   C,S,M,i;
int16 Amp;
float Amperaje;

#int_timer0
void timer() 
{
   if(Operando)
   {
      set_timer0(61);
      if(C==0)
      {
         C=20;
         SegPar++;
         if(S==0)
         {
            S=60;
            M--;
         }
         S--;
      }
      C--;
   }
}
void Pitido()
{
   for(i=0;i<20;i++)
   {
      output_toggle(PIN_A2);
      delay_ms(2);
   }
}
void LeerADC()
{
   Amp=read_adc();
   Amperaje=(Amp*5.0)/1024.0;
}
void Mostrar()
{
   lcd_gotoxy(1,1);
   if(SegPar)
   {
      printf(lcd_putc,"FOOTSHOWER   ON ");
   }
   if(!SegPar)
   {
      if(Zapper)
      {
         printf(lcd_putc,"ZAPPER       ON ");
         if(!PWM)
         {
            setup_timer_2(T2_DIV_BY_1,32,1);   // PWM frequency to 121.2KHz
            set_pwm1_duty(128);
            setup_ccp1(CCP_PWM);
            PWM=1;
         }
      }
      if(!Zapper)
      {
         printf(lcd_putc,"ZAPPER       OFF");
         setup_ccp1(CCP_OFF);
         setup_timer_2(T2_DISABLED,1,1);
         PWM=0;
      }
   }
   lcd_gotoxy(1,2);
   if(C>10)
   {
      printf(lcd_putc,"TMP %02d %02d   %1.1fA",M,S,Amperaje);
   }
   if(C<10)
   {
      printf(lcd_putc,"TMP %02d:%02d   %1.1fA",M,S,Amperaje);
   }
}
void Exceso()
{
   while(Amperaje>1.4) // EXCESO DE SOLUCION
   {
      lcd_gotoxy(1,1);
      LeerADC();
      printf(lcd_putc,"EXCESO  SOLUCION\n      %1.1fA      ",Amperaje);
      //Pitido();
   }
}
void Pediluvio()
{
   lcd_gotoxy(1,1);
   printf(lcd_putc,"*  FOOTSHOWER  *\n      %1.1fA",Amperaje);
   Exceso();
}
void Estado()
{
   if(!BInicio) // Detenido
   {
      while(!BInicio){}
      Operando=0;
      Pitido();
      printf(lcd_putc,"\fPROCESO DETENIDO");
      delay_ms(2000);
   }
   if(!BZapper) // Zapper Apagado/Encendido
   {
      Pitido();
      Zapper++;
      while(!BZapper){}
   }
}
void Fin()
{
   if(!S && !M)
   {
      Rele=0;
      printf(lcd_putc,"\f FIN DE TERAPIA");
      Pitido();
      delay_ms(3000);
      Reset_cpu();
   }
}
void PediluvioOperando()
{
   while(Operando)
   {
      Mostrar();
      Estado();
      LeerADC();
      Exceso();
      Fin();
   }
}
void main()
{
   lcd_init();
   set_tris_a(0b00110001);       // Puerto A como Entrada
   set_tris_b(0x00);             // Puerto B como Salida
   output_b(0x00);               // Puerto B inicia en cero
   setup_adc_ports(sAN0);        // Habilito AN0 como pueto ADC
   setup_adc(adc_clock_internal|adc_tad_mul_0);   // ADC con fuente de reloj interno
   set_adc_channel(0);
   enable_interrupts(global|int_timer0);
   setup_timer_0(T0_INTERNAL|T0_DIV_256|T0_8_BIT);
   set_timer0(61);
   Operando=0;
   SegPar=0;
   Zapper=1;
   Rele=1;
   printf(lcd_putc,"* ZELECTRON CA *");
   delay_ms(1500);
   printf(lcd_putc,"\f");
   while(true)
   {
      LeerADC();
      Pediluvio();
      if(!BInicio)
      {
         Pitido();
         while(!BInicio){}
         C=S=PWM=0;
         M=60;
         Operando=1;
         PediluvioOperando();
      }
   }
}
