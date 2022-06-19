#include <mega328p.h>
#include <stdio.h>    	// Standard Input/Output functions

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include <delay.h>	//singura bibliotecă adăugată de mine 

// Declare your global variables here
#define false 0		
#define true 1		
#define High0  2500	//timpul High pentru PWM servomotor cand mut axul in pozitia 0
#define Low0 17500	//timpul Low pentru PWM servomotor cand mut axul in pozitia 0
#define High1  500	//timpul High pentru PWM servomotor cand mut axul in pozitia 1
#define Low1 19500	//timpul Low pentru PWM servomotor cand mut axul in pozitia 1
#define halfPeriod 500  //= 1000000L/frequency/2; //500 pt 1kHz, 1000 pt 500hz  

unsigned long Time = 0, currentTime = 4000, a1_previousTime=0, a2_previousTime = 0, a3_previousTime = 0,  previousTime = 0;       
float set_temp[] = {16.5, 16.5, 16.5}, t1 = 17.1, t2 = 17.2, t3 = 17.3 , decalaj_Mod = 0.5;   //t[] ={ 29.1, 29.2, 29.3} ;                    
bit heat = 0, heat1 = 0, heat2 = 0, heat3 = 0, window1 = 0, window2 = 0, window3 = 0, alarm1 = 0, alarm2 = 0, alarm3 = 0;
unsigned char h1 = 91, h2 = 92, h3 = 93;
unsigned char i = 0, cnt0 = 0; 
unsigned char to_esp[30], from_esp[16], Mod_Incalzire = 0, Mod_Noapte = 0;

//functii//////////////////////////////////////
unsigned long mili ();
unsigned char rxpushchar();
void txpopcharar(unsigned char str[]);  
void workwork();
void BUZZ(volatile unsigned char * PORT_X,  unsigned char  Nr_Pin, unsigned short duration);
void Achizitie_DHT11(volatile unsigned char * DDR_X, volatile unsigned char * PORT_X, volatile unsigned char * PIN_X,  unsigned char  Numar_Pin, unsigned char *h, float *t);
void Robinet(volatile unsigned char * PORT_X, unsigned char Nr_Pin, unsigned char sens);
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



#define DATA_REGISTER_EMPTY (1<<UDRE0)
#define RX_COMPLETE (1<<RXC0)
#define FRAMING_ERROR (1<<FE0)
#define PARITY_ERROR (1<<UPE0)
#define DATA_OVERRUN (1<<DOR0)

// USART Receiver buffer
#define RX_BUFFER_SIZE0 16
char rx_buffer0[RX_BUFFER_SIZE0];
unsigned char rx_wr_index0=0, rx_rd_index0=0;// #if RX_BUFFER_SIZE0 <= 256   else int
unsigned char rx_counter0=0; //#if RX_BUFFER_SIZE0 < 256 else int
bit rx_buffer_overflow0;  // This flag is set on USART Receiver buffer overflow

interrupt [USART_RXC] void usart_rx_isr(void) { //USART Receiver interrupt service routine
    char status, data;
    status=UCSR0A;   // registrul de status ce contine flagurile de eroare
    data=UDR0;       // registrul in care se stocheaza datele Receptionate
    if ((status & (FRAMING_ERROR | PARITY_ERROR | DATA_OVERRUN))==0){ //daca nu sunt erori de receptie
        rx_buffer0[rx_wr_index0++]=data;                              //scrie in buffer valoarea primita pe RX
        if (rx_wr_index0 == RX_BUFFER_SIZE0)                          //daca am ajuns la final de buffer
        	rx_wr_index0=0;  										  //overflow, scrie pe pozitia 0 data viitoare
        if (++rx_counter0 == RX_BUFFER_SIZE0) { 					  //daca bufferul e plin
            rx_counter0=0;   										  //setare flag buffer plin 
            rx_buffer_overflow0=1;                              	  //setare flag buffer plin
        }
    }
}

interrupt [TIM0_COMPA] void timer0_compa_isr(void) { // Timer 0 output compare A interrupt service routine 
    Time += 16; //cand timerul ajunge la valorea F9 corespunzatoare intreruperii, inseamna cã au trecut 16ms si incrementez timpul curent de functionare
}


void main(void) {

#pragma optsize-
CLKPR=(1<<CLKPCE);   // Crystal Oscillator division factor: 1
CLKPR=(0<<CLKPCE) | (0<<CLKPS3) | (0<<CLKPS2) | (0<<CLKPS1) | (0<<CLKPS0);
#ifdef _OPTIMIZE_SIZE_
#pragma optsize+
#endif

DDRB=(0<<DDB7) | (0<<DDB6) | (0<<DDB5) | (0<<DDB4) | (0<<DDB3) | (1<<DDB2) | (1<<DDB1) | (0<<DDB0);
PORTB=(0<<PORTB7) | (0<<PORTB6) | (0<<PORTB5) | (0<<PORTB4) | (0<<PORTB3) | (0<<PORTB2) | (0<<PORTB1) | (0<<PORTB0);
DDRC=(0<<DDC6) | (0<<DDC5) | (0<<DDC4) | (0<<DDC3) | (0<<DDC2) | (0<<DDC1) | (1<<DDC0);
PORTC=(0<<PORTC6) | (0<<PORTC5) | (0<<PORTC4) | (1<<PORTC3) | (1<<PORTC2) | (1<<PORTC1) | (0<<PORTC0);       //        c1c2c3 1 internal pulup
DDRD=(1<<DDD7) | (1<<DDD6) | (0<<DDD5) | (1<<DDD4) | (1<<DDD3) | (0<<DDD2) | (0<<DDD1) | (0<<DDD0);
PORTD=(0<<PORTD7) | (0<<PORTD6) | (0<<PORTD5) | (0<<PORTD4) | (0<<PORTD3) | (0<<PORTD2) | (0<<PORTD1) | (0<<PORTD0);

// Timer/Counter 0 initialization// Clock source: System Clock// Clock value: 15.625 kHz// Mode: CTC top=OCR0A// OC0A output: Disconnected// OC0B output: Disconnected// Timer Period: 16 ms
TCCR0A=(0<<COM0A1) | (0<<COM0A0) | (0<<COM0B1) | (0<<COM0B0) | (1<<WGM01) | (0<<WGM00);
TCCR0B=(0<<WGM02) | (1<<CS02) | (0<<CS01) | (1<<CS00);
TCNT0=0x06;    OCR0A=0xF9;    OCR0B=0x00;

// Timer/Counter 1 initialization
TCCR1A=(0<<COM1A1) | (0<<COM1A0) | (0<<COM1B1) | (0<<COM1B0) | (0<<WGM11) | (0<<WGM10);
TCCR1B=(0<<ICNC1) | (0<<ICES1) | (0<<WGM13) | (0<<WGM12) | (0<<CS12) | (0<<CS11) | (0<<CS10);
TCNT1H=0x00;  TCNT1L=0x00;  ICR1H=0x00;  ICR1L=0x00; OCR1AH=0x00;  OCR1AL=0x00;  OCR1BH=0x00;  OCR1BL=0x00;

// Timer/Counter 2 initialization
ASSR=(0<<EXCLK) | (0<<AS2);
TCCR2A=(0<<COM2A1) | (0<<COM2A0) | (0<<COM2B1) | (0<<COM2B0) | (0<<WGM21) | (0<<WGM20);
TCCR2B=(0<<WGM22) | (0<<CS22) | (0<<CS21) | (0<<CS20);
TCNT2=0x00;    OCR2A=0x00;    OCR2B=0x00;

// Timer/Counter 0 Interrupt(s) initialization
TIMSK0=(0<<OCIE0B) | (1<<OCIE0A) | (0<<TOIE0);       ///(1<<TOIE0)//////
// Timer/Counter 1 Interrupt(s) initialization
TIMSK1=(0<<ICIE1) | (0<<OCIE1B) | (0<<OCIE1A) | (0<<TOIE1);
// Timer/Counter 2 Interrupt(s) initialization
TIMSK2=(0<<OCIE2B) | (0<<OCIE2A) | (0<<TOIE2);

// External Interrupt(s) initialization
// INT0: Off  // INT1: Off
// Interrupt on any change on pins PCINT0-7: Off
// Interrupt on any change on pins PCINT8-14: Off
// Interrupt on any change on pins PCINT16-23: Off
EICRA=(0<<ISC11) | (0<<ISC10) | (0<<ISC01) | (0<<ISC00);
EIMSK=(0<<INT1) | (0<<INT0);
PCICR=(0<<PCIE2) | (0<<PCIE1) | (0<<PCIE0);

// USART initialization
// Communication Parameters: 8 Data, 1 Stop, No Parity  // USART Receiver: On
// USART TransmiHigh1er: On      // USART0 Mode: Asynchronous    // USART Baud Rate: 9600
UCSR0A=(0<<RXC0) | (0<<TXC0) | (0<<UDRE0) | (0<<FE0) | (0<<DOR0) | (0<<UPE0) | (0<<U2X0) | (0<<MPCM0);
UCSR0B=(1<<RXCIE0) | (0<<TXCIE0) | (0<<UDRIE0) | (1<<RXEN0) | (1<<TXEN0) | (0<<UCSZ02) | (0<<RXB80) | (0<<TXB80);
UCSR0C=(0<<UMSEL01) | (0<<UMSEL00) | (0<<UPM01) | (0<<UPM00) | (0<<USBS0) | (1<<UCSZ01) | (1<<UCSZ00) | (0<<UCPOL0);
UBRR0H=0x00;        UBRR0L=0x67;

// Analog Comparator initialization      // Analog Comparator: Off
// The Analog Comparator's positive input is        // connected to the AIN0 pin
// The Analog Comparator's negative input is              // connected to the AIN1 pin
ACSR=(1<<ACD) | (0<<ACBG) | (0<<ACO) | (0<<ACI) | (0<<ACIE) | (0<<ACIC) | (0<<ACIS1) | (0<<ACIS0);
ADCSRB=(0<<ACME);
// Digital input buffer on AIN0: On            // Digital input buffer on AIN1: On
DIDR1=(0<<AIN0D) | (0<<AIN1D);

// ADC initialization    // ADC disabled
ADCSRA=(0<<ADEN) | (0<<ADSC) | (0<<ADATE) | (0<<ADIF) | (0<<ADIE) | (0<<ADPS2) | (0<<ADPS1) | (0<<ADPS0);

// SPI initialization      // SPI disabled
SPCR=(0<<SPIE) | (0<<SPE) | (0<<DORD) | (0<<MSTR) | (0<<CPOL) | (0<<CPHA) | (0<<SPR1) | (0<<SPR0);

// TWI initialization          // TWI disabled
TWCR=(0<<TWEA) | (0<<TWSTA) | (0<<TWSTO) | (0<<TWEN) | (0<<TWIE);


	
	
///////////////////////////////////////////////////////////////////
Robinet(&PORTD, 4, false); //se duc servomoarele in pozitia OFF
Robinet(&PORTD, 7, false); 
Robinet(&PORTB, 2, false);
//PORTC &= ~(1 << 0);

// Global enable interrupts
#asm("sei")

while (1)      {
    
    if(rx_counter0) { 			//daca se receptioneaza
        delay_ms(25); 			//astept sa se primeasca tot pachetul 
        cnt0 = rx_counter0;     	//tin minte cate caractere s-au receptionat deoarece rx_counter0 se va modifica in timpul citirii buffer-ului
        for (i = 0; i < cnt0; i++) { 		// preiau tot rx buffer-ul disponibil 
            from_esp[i] = rxpushchar(); 
        }
        if(cnt0==14 && from_esp[2] == '.'){	    //daca am primit corect(3*4+2) si daca al treilea caracter e punct(virgula); 
        										// esp trimite caractere aleator cand se reseteaza si nu trebuie interpretate
                                                // deoarece deregleaza variabilele din AVR  
                                                
    		Mod_Noapte = from_esp[13] - 48;  	//extrage flag mod noapte  (scad 48 pt a salva valoarea ca si nr intreg, ci nu cod ASCII)
            
            for(i = 0; i < 3; i++) {           	//extrage 3 temperaturi de forma XX.X (3 * 4 = 12 caractere)
                set_temp[i] = (float)((from_esp[4 * i] - 48) * 100 + (from_esp[1 + 4 * i] - 48) * 10 + (from_esp[3 + 4 * i] - 48)) / 10; 
                
                if(Mod_Noapte == 1)            	
                	set_temp[i] -= 1;           //daca mod noapte activ atunci temp mai joasa cu 1 grad
            }
            
            Mod_Incalzire = from_esp[12] - 48;  //extrag modul setat pentru incalzire
            if(Mod_Incalzire == 0) 
            	decalaj_Mod = 0.5;          	//mod NORMAL interval de 1 grad [+-0.5 fata de temp setata]
            if(Mod_Incalzire == 1) 
            	decalaj_Mod = 0.3;         		//mod CONFORT interval de 0.6 grade [+-0.3 fata de temp setata]
            if(Mod_Incalzire == 2) 
            	decalaj_Mod = 0.8;         		//mod ECO interval de 1.6 grade [+-0.8 fata de temp setata]
            
            workwork();     					//prelucrez imediat datele primite.
        }                                                                                      
        
    }
    
    currentTime = mili();                       //preiau timpul curent de functionare
    if( currentTime - previousTime > 5000) { 	//pentru a prelucra si trimite date odata la 5s 
        workwork();
    }

  
    //gestionare alarmare sonora pt cele 3 difuzoare pasive conectate la pinii D3, D6, B1
    if(alarm1) {
        if(currentTime - a1_previousTime > 500) {
            BUZZ(&PORTD, 3, 500);
            a1_previousTime = currentTime+500;
        }
    }
    if(alarm2) {
        if(currentTime - a2_previousTime > 500) {
            BUZZ(&PORTD, 6, 500);
            a2_previousTime = currentTime+500;
        }
    }
    if(alarm3) 
        if(currentTime - a3_previousTime > 500) {
            BUZZ(&PORTB, 1, 500);
            a3_previousTime = currentTime+500;
        }
    }
}//loop
}//main

void workwork() {
	Achizitie_DHT11(&DDRD, &PORTD, &PIND, 2, &h1, &t1);
        Achizitie_DHT11(&DDRD, &PORTD, &PIND, 5, &h2, &t2);
        Achizitie_DHT11(&DDRB, &PORTB, &PINB, 0, &h3, &t3);
        
        if(t1 <= set_temp[0] - decalaj_Mod && heat1 == false) {
            heat1 = true;
            Robinet(&PORTD, 4, true);	//D4 servo1
        }
        else if(t1 >= set_temp[0] + decalaj_Mod && heat1 == true) {
            heat1 = false;
            Robinet(&PORTD, 4, false); //D4 servo1
        }              
        if(t2 <= set_temp[1] - decalaj_Mod && heat2 == false) {
            heat2 = true;
            Robinet(&PORTD, 7, true);	//D7 servo2
        }
        else if( t2 >= set_temp[1] + decalaj_Mod && heat2 == true) {
            heat2 = false;
            Robinet(&PORTD, 7, false);	//D7 servo2
        }
        if(t3 <= set_temp[2] - decalaj_Mod && heat3 == false) {
            heat3 = true;
            Robinet(&PORTB, 2, true);	//B2 servo3
        }
        else if( t3 >= set_temp[2] + decalaj_Mod && heat3 == true) {
            heat3 = false;
            Robinet(&PORTB, 2, false);	//B2 servo3
        }
        
                      
        if ( (heat1 || heat2 || heat3) && heat == false){
            heat = true;
            PORTC |= (1<<0); 		//A0 HIGH
        }else if(heat1 == false && heat2 == false && heat3 == false && heat == true){
            heat = false;
        	PORTC &= ~(1<<0);	//A0 LOW
        }               
        
        window1 = !(PINC & (1<<3));  		//c3
        window2 = !(PINC & (1<<2)); 		//c2
        window3 = !(PINC & (1<<1));		//c1
        if(window1 && heat1 && !alarm1) {
            alarm1 = true;
        }else if( (!window1 || !heat1) && alarm1) {
            alarm1 = false;
        }
        if(window2 && heat2 && !alarm2) {
            alarm2 = true;
        }else if( (!window2 || !heat2) && alarm2) {
            alarm2 = false;
        }
        if(window3 && heat3 && !alarm3) {
            alarm3 = true;
        }else if( (!window3 || !heat3) && alarm3) {
            alarm3 = false;
        }

        sprintf(to_esp, "%d%d%d%.1f%.1f%.1f%d%d%d%d%d%d%d%d%d%d\r\n", h1, h2, h3, t1, t2, t3, heat, heat1, heat2, heat3, window1, window2, window3, alarm1, alarm2, alarm3);
        txpopcharar(&to_esp);   	//trimit referinta ( doar adresa,  nu tot stringul)
        previousTime = currentTime;
}

unsigned long mili()  { //functie ce returneaza valoarea timpului[ms] parcurs de la pornire
    unsigned long m;
    #asm("cli")  		// opresc intreruperile momentan 
    m = Time;    		// pt a nu incrementa timpul in timpul citirii 
    #asm("sei")  		// reactivare intreruperi
    return m;    
}

unsigned char rxpushchar() {			//functie ce returneaza urmatorul element din bufferul rx
    char data;                            
    while (rx_counter0==0);               	//daca bufferul e gol, astept sa primeasca ceva intai
    data = rx_buffer0[rx_rd_index0++];		//data primeste cea mai veche valoare din buffer
    if (rx_rd_index0 == RX_BUFFER_SIZE0) 	//daca am citit tot buffer-ul
        rx_rd_index0=0;					//resetare index citire
    #asm("cli")					//dezact intreruperi ca sa nu se modifice counter in timp ce il decrementez
    --rx_counter0;				//am citit o valoare din buffer => -1 element
    #asm("sei")					//reactivare intrerupero=i
    return data;				//returneaza caracterul extras din buffer
}

void txpopcharar(unsigned char str[]) {  //*
    unsigned char idx = 0;
    while(str[idx] != 0)  {  			//daca char = 0 atunci am terminat transmisia
    	while (!( UCSR0A & (1<<UDRE0)));		// astept txbufferReady
        UDR0 = str[idx++];  				// odata ce caracterele ajung în buffer, ele sunt trimise 
        }
}

void Robinet(volatile unsigned char * PORT_X, unsigned char Nr_Pin, unsigned char sens) {
    if(sens == 1){				//daca se doreste deschiderea robinetului
        for(i=0;i<=100;i++){ 				//50Hz=>20ms*100=2s durata de deschidere robinet
            *PORT_X |= (1 << Nr_Pin); 				//trimit "1" catre servomotor
            delay_us(High1);    				//pentru High1 = 500us
            *PORT_X &= ~(1 << Nr_Pin);				//trimit "0" catre servomotor
            delay_us(Low1);   					//pentru Low1 = 19500us
        }
    }else{					//daca se doreste inchiderea robinetului
        for(i=0;i<=100;i++){ 				//2s durata de inchidere robinet
            *PORT_X |= (1 << Nr_Pin); 				//trimit "1" catre servomotor
            delay_us(High0);    				//pentru High0 = 500us
            *PORT_X &= ~(1 << Nr_Pin);				//trimit "0" catre servomotor
            delay_us(Low0);   					//pentru Low1 = 19500us
        }
    }
}//Robinet       
 
void BUZZ(volatile unsigned char * PORT_X,  unsigned char  Nr_Pin, unsigned short duration) {
    unsigned long startTime=mili();
    while (mili()-startTime < duration) {
		*PORT_X |= (1 << Nr_Pin); 			//digitalWrite(pin,HIGH);
        delay_us(halfPeriod);
        *PORT_X &= ~(1 << Nr_Pin); 				//digitalWrite(pin,LOW);
        delay_us(halfPeriod);
    }
}//buzz

//Achizitie_DHT11(DDRC, PORTC, PINC, PC1, h, t); 
void Achizitie_DHT11(volatile unsigned char * DDR_X,						//reg responsbil de tipul pinilor: intrare sau iesire
                   volatile unsigned char * PORT_X, 						//reg responsabil de valoarea iesirii
                   volatile unsigned char * PIN_X, 						 //reg responsabil de valoarea intrarii
                   unsigned char  Numar_Pin, unsigned char* h, float *t) {
    unsigned char byte_index, bit_index, buffer = 0, timeout;
    *DDR_X |= (1 << Numar_Pin); 								//setez pinul ca si iesire pentru a trimite semnalul de interogare
    *PORT_X &= ~(1 << Numar_Pin); 								//trimit semnal LOW cu rol de interogare
    delay_ms(20); 										//astept ca senzorul sa primeasca semnalul de interogare >18ms
    *DDR_X &= ~(1 << Numar_Pin); 								//setez pinul ca si intrare pentru a primi date de la senzor
    delay_us(32); 										//astept 20-40 us
    delay_us(82); 										//senzorul trimite "0" 80ms => semnal de raspuns
    delay_us(82); 										//senzorul trimite "1" 80ms dupa semnal de raspuns
    for(byte_index=0; byte_index<4; byte_index++){						//senzorul incepe sa trimita datele
        buffer = 0;		
        for(bit_index=0; bit_index<8; bit_index++){		
            timeout = 0;
            while((~*PIN_X & (1 << Numar_Pin)) && timeout < 25){//while "0"
                timeout++;									//senzorul trimite "0" inainte de fiecare bit 26-28us	
                delay_us(4);									//daca trimite "0" mai mult de 100us renunt la achizitie
            }
            delay_us(40); 									//citesc semnalul primit dupa 40 ms
            if(*PIN_X & (1 << Numar_Pin)){							//daca inca e High ~> "1"
                buffer |= 1 << (7-bit_index);							//adaug "1" la buffer pe pozitia curenta
                delay_us(40);									//daca a fost "1" astept sa coboare in "0"
            }//alHigh0el e  "0" deci continui cu urmatorul bit    
        }
        if(buffer){										//daca am primit date le salvez corespunzator
            if(byte_index == 0) *h = buffer;             					//parte intreaga h
                else if(byte_index == 2) *t = buffer;    					//parte intreaga t
                    else if(byte_index == 3) *t = (*t * 10 + buffer) / 10 ; // parte zecimala t; 
        }											// impart la 10 sa fie 
    }												// de forma xx.x
}//Achizitie_DHT11 
