#include <msp430g2553.h>

#define RX BIT1
#define TX BIT2
#define SEL1 BIT4
#define SEL0 BIT3
#define trigger BIT4
#define echo BIT5
#define LED0 BIT0
#define LED1 BIT6
#define CENTRO 0
#define DIREITA 1
#define ESQUERDA 2
#define EMBAIXO 3


int miliseconds;

long sensor;
#define BAUD_9600   0
#define BAUD_19200  1
#define BAUD_38400  2
#define BAUD_56000  3
#define BAUD_115200 4
#define BAUD_128000 5
#define BAUD_256000 6
#define NUM_BAUDS   7

typedef struct sensoresUltrassonicos
  {
    int Medicao[4];// Medi��es para entrada do filtro m�dia m�vel
    int valorFinal; // Resultado do filtro m�dia m�vel

  }sensoresUltrassonicos;


void Send_Data(unsigned char c);
void Send_Int(int n);
void Send_String(char str[]);
void Init_UART(unsigned int baud_rate_choice);
void filtro_Media_movel(sensoresUltrassonicos *sensor);





int main(void)
{
    sensoresUltrassonicos distancia[4] = {{.Medicao = {0,0,0,0}, .valorFinal = 0},{.Medicao = {0,0,0,0}, .valorFinal = 0},{.Medicao = {0,0,0,0}, .valorFinal = 0},{.Medicao = {0,0,0,0}, .valorFinal = 0}};
    volatile int i,j = 0;
    WDTCTL = WDTPW + WDTHOLD;
    BCSCTL1 = CALBC1_1MHZ;
    DCOCTL = CALDCO_1MHZ;
    P2DIR |= SEL1;                            // P1.0 as output for LED
    P2OUT &= ~SEL1;
    P1DIR |= SEL0;
    P1OUT &= ~SEL0;
    P1DIR |= LED0;
    P1DIR |= LED1;
    P1OUT &= ~LED0;// P1.0 as output for LED
    P1OUT &= ~LED1;

    CCTL0 = CCIE;                             // CCR0 interrupt enabled
    CCR0 = 1000;                  // 1ms at 1mhz
    TACTL = TASSEL_2 + MC_1;                  // SMCLK, upmode
    CCTL0 = CCIE;                             // CCR0 interrupt enabled
    _BIS_SR(GIE);
    Init_UART(BAUD_9600);
    volatile int nroSensor = 0;
    volatile int nroMedicao = 0;

     //inicializar_sensores

    while(1)
    {

        switch(nroSensor)
        {
            case CENTRO:

                P2OUT &= ~SEL1;
                P1OUT &= ~SEL0;
            break;
            case DIREITA:
                P2OUT &= ~SEL1;
                P1OUT |= SEL0;
            break;
            case ESQUERDA:
                P1OUT &= ~SEL0;
                P2OUT |= SEL1;
            break;

        }
        P1IE &= ~0x01;       // disable interupt
        P1DIR |= trigger;          // trigger pin as output
        P1OUT |= trigger;          // generate pulse
        __delay_cycles(10);             // for 10us
        P1OUT &= ~trigger;                 // stop pulse
        P1DIR &= ~echo;         // make pin P1.2 input (ECHO)
            P1IFG = 0x00;                   // clear flag just in case anything happened before
        P1IE |= echo;           // enable interupt on ECHO pin
        P1IES &= ~echo;         // rising edge on ECHO pin
        distancia[nroSensor].Medicao[nroMedicao] = sensor/58;           // converting ECHO lenght into cm
                Send_Int(distancia[nroSensor].valorFinal);
                Send_String("\n");
                Send_Int(distancia[nroSensor].valorFinal);
                Send_String("\n");
                Send_Int(distancia[nroSensor].valorFinal);
                Send_String("\n");

                nroMedicao++;

            filtro_Media_movel(&distancia[nroSensor]);
            if(nroMedicao >= 3)
            {
                nroMedicao = 0;
                nroSensor++;
                nroSensor > 3 ?  (nroSensor = 0) : (nroSensor = nroSensor);

            }

            if(distancia[2].valorFinal < 30)
                {P1OUT |= LED1;
                P1OUT |= LED0;}
            else{
                P1OUT &= ~LED1;
                P1OUT &= ~LED0;}
            if(distancia[0].valorFinal  < 30)
                P1OUT |= LED0;
            else
                P1OUT &= ~LED0;

            if(distancia[1].valorFinal  < 30)
                P1OUT |= LED1;
            else
                P1OUT &= ~LED1;




}
}
void filtro_Media_movel(sensoresUltrassonicos *sensor)
{
    sensor->valorFinal= (sensor->Medicao[0]+ sensor->Medicao[1]+ sensor->Medicao[2] + sensor->Medicao[3])/4;
}

void Send_Data(unsigned char c)
{
    while((IFG2&UCA0TXIFG)==0);
    UCA0TXBUF = c;
}

void Send_Int(int n)
{
    int casa, dig;
    if(n==0)
    {
        Send_Data('0');
        return;
    }
    if(n<0)
    {
        Send_Data('-');
        n = -n;
    }
    for(casa = 1; casa<=n; casa *= 10);
    casa /= 10;
    while(casa>0)
    {
        dig = (n/casa);
        Send_Data(dig+'0');
        n -= dig*casa;
        casa /= 10;
    }
}

void Send_String(char str[])
{
    int i;
    for(i=0; str[i]!= '\0'; i++)
        Send_Data(str[i]);
}

void Init_UART(unsigned int baud_rate_choice)
{
    unsigned char BRs[NUM_BAUDS] = {104, 52, 26, 17, 8, 7, 3};
    unsigned char MCTLs[NUM_BAUDS] = {UCBRF_0+UCBRS_1,
                                        UCBRF_0+UCBRS_0,
                                        UCBRF_0+UCBRS_0,
                                        UCBRF_0+UCBRS_7,
                                        UCBRF_0+UCBRS_6,
                                        UCBRF_0+UCBRS_7,
                                        UCBRF_0+UCBRS_7};
    if(baud_rate_choice<NUM_BAUDS)
    {
        // Habilita os pinos para transmissao serial UART
        P1SEL2 = P1SEL = RX+TX;
        // Configura a transmissao serial UART com 8 bits de dados,
        // sem paridade, comecando pelo bit menos significativo,
        // e com um bit de STOP
        UCA0CTL0 = 0;
        // Escolhe o SMCLK como clock para a UART
        UCA0CTL1 = UCSSEL_2;
        // Define a baud rate
        UCA0BR0 = BRs[baud_rate_choice];
        UCA0BR1 = 0;
        UCA0MCTL = MCTLs[baud_rate_choice];
    }
}

#pragma vector=PORT1_VECTOR
__interrupt void Port_1(void)
{
    if(P1IFG&echo)  //is there interrupt pending?
        {
          if(!(P1IES&echo)) // is this the rising edge?
          {
            TACTL|=TACLR;   // clears timer A
            miliseconds = 0;
            P1IES |= echo;  //falling edge
          }
          else
          {
            sensor = (long)miliseconds*1000 + (long)TAR;    //calculating ECHO lenght

          }
    P1IFG &= ~echo;             //clear flag
    }
}
#pragma vector=TIMER0_A0_VECTOR
__interrupt void Timer_A (void)
{
  miliseconds++;
}
