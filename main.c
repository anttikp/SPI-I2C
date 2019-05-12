/* 
create: 12.4.2019
autor: Miriam Nold
PSoC SPI & I2C
Exercise 5

description: 	
	Read an analog, OneWire, I2C and SPI value and shows it on the LCD.
	The PSoC is controlled by the com port of the PC with an
	UART communication between Pc and PSoC
*/

#include <m8c.h>        // part specific constants and macros
#include "PSoCAPI.h"    // PSoC API definitions for all User Modules
#include <stdlib.h>
#include <stdio.h>

//parameter for LCD
char intToString[10];

//parameter for Analog Temperature
long int stemp = 0;				// the sum of added data that a gave from the EzADC
long int ntemp = 0;				// the number of added data (EzADC
long int valueAnalog = 0;		// the average of the data

//Paramter for OneWire
long int valueOneWire = 0;		// the average of the data
int readOneWire = 0;

// Parameter for TX8SW
char *strPtr;					// Parameter pointer to read the Input

// Parameter for I2C
BYTE valueI2C;

//Parameter for SPIM
char cRXDataSPI;			// 1. Byte of data from the SPI ADC
char cRXData1SPI;			// 2. Byte of data from SPI ADC
int togetherSPI;			// 1. and 2. Byte combined to 16 bits
int maskSPI = 0b0000111111111111;	// to remove the first 2 unknown bits of the ADC
long valueSPI;				// final SPI result
long longValueSPI;			// for calculation perpes

//Functions
void analogValueFunc(void);
void oneWireValue(void);
void i2cValue(void);
void spiValue(void);

void lcdClean(void);
void lcdOutput(int yInt,int xInt,int contentInt);


void main(void)
{
	M8C_EnableGInt ; // enable global interrupts 
	  
	PGA_Start(PGA_HIGHPOWER);      // Turn on the PGA
    	LPF2_Start(LPF2_HIGHPOWER);    // Turn on the LPF
	
	Counter8_WritePeriod(0x07);	    // set period to eight clocks
   	Counter8_WriteCompareValue(0x03);   // generate a 50% duty cycle
    	Counter8_EnableInt();               // ensure interrupt is enabled
    	Counter8_Start();                   // start the counter!
	
	LCD_Start();               	    // Initialize LCD
	OneWire_Start();          	    // Initialize 1-Wire pin
	
	I2Cm_Start();
	
	SPIM_Start(SPIM_SPIM_MODE_0  | SPIM_SPIM_MSB_FIRST);
	
	RX8_EnableInt();			// Enable the RX8 interrupt 
    	RX8_Start(RX8_PARITY_NONE); 		// Set the parity of the RX8 receiver and enable the
	TX8SW_Start();
	TX8SW_CPutString("\r Welcome\r\n");
	


	
	while (1){
		if(RX8_bCmdCheck) {  // Wait for command from PC 
			
			if(strPtr = RX8_szGetParam()) {					// More than delimiter"
            			TX8SW_CPutString("\r\nFound valid command =>");		// verify Input
				TX8SW_PutString(strPtr);   				// Print out command
				TX8SW_CPutString("\r\n");

				if ((strPtr[0] == 'a') || (strPtr[0] == 'A')){ 		// if receive "a" or "A"
					EzADC_Stop();					// Stop ADC, so that the LCD output is not over written 
					TX8SW_CPutString("\r NoldPSoClab4 \r\n");
					lcdClean();
					LCD_Position(0,2);				// Place LCD cursor at row 0, col 0
					LCD_PrCString("NoldPSoClab4");			// Print "NoldPSoClab4" on the LCD
				}
				if ((strPtr[0] == 's') || (strPtr[0] == 'S')) {		// if receive "s" or "S"
					EzADC_Stop();					// Stop ADC
					EzADC_Start(EzADC_HIGHPOWER);       		// Apply power to ADC
					lcdClean();					// clean LCD
					
					// to calculate one sample of the analog Value
					EzADC_GetSamples(1);				// to show just 1 Sample (Analog are 2 Samples)
					valueAnalog = EzADC_iGetDataClearFlag(); 
					valueAnalog = (valueAnalog*400)/(4096);
					LCD_Position(0,0);
					LCD_PrCString("Analog");
					lcdOutput(0,8,valueAnalog);			// call the function for LCD output
					
					readOneWire = 2;				// Analog is calculating the average from 50 samples
				}
				if ((strPtr[0] == 'c') || (strPtr[0] == 'c')){
					EzADC_Stop();					// Stop ADC
					lcdClean();					// clean LCD
					
					// I2C
					i2cValue();
					LCD_Position(0,9);
					LCD_PrCString("I2C:");
					lcdOutput(0,13,valueI2C);
					//SPI
					spiValue();
					LCD_Position(1,9);
					LCD_PrCString("SPI");
					lcdOutput(1,13,valueSPI);
					
					EzADC_Start(EzADC_HIGHPOWER);       		// Apply power to ADC
					EzADC_GetSamples(0);                		// Have ADC run continuously
					
				}
				if ((strPtr[0] == 'x') || (strPtr[0] == 'X')){
					EzADC_Stop();					// Stop ADC
					lcdClean();					// clean LCD
					LCD_Position(0,0);				// LCD output
					LCD_PrCString("Stop sampling");
					TX8SW_CPutString("\r\nStop sampling\r\n ");
				}
			}
		RX8_CmdReset();                        				// Reset command buffer
		} 
		
		if (ntemp >= 50){			//by 50 data the average will be calculated
			
			analogValueFunc();
			LCD_Position(0,0);
			LCD_PrCString("Analog");
			lcdOutput(0,7,valueAnalog);	// call the function for LCD output
		}
		
		
		
		if( readOneWire == 2 ){
			
			readOneWire = 0;
			
			oneWireValue();
			LCD_Position(0,1);
			LCD_PrCString("OneWire:");
			lcdOutput(1,9,valueOneWire);	// call the function for LCD output
			
			TX8SW_CPutString("\r\n { \r\n");
			TX8SW_CPutString("\r\nAnalog: ");
			TX8SW_PutString(itoa(intToString,valueAnalog,10));
			TX8SW_CPutString("\r\n }\r\n");
			TX8SW_CPutString("\r\n { \r\n");
			TX8SW_CPutString("OneWire: ");
			TX8SW_PutString(itoa(intToString,valueOneWire,10));
			TX8SW_CPutString("\r\n }\r\n");
			TX8SW_CPutString("\r\n { \r\n");
			TX8SW_CPutString("I2C: ");
			TX8SW_PutString(itoa(intToString,valueI2C,10));
			TX8SW_CPutString("\r\n }\r\n");
			TX8SW_CPutString("\r\n { \r\n");
			TX8SW_CPutString("SPI: ");
			TX8SW_PutString(itoa(intToString,valueSPI,10));
			TX8SW_CPutString("\r\n }\r\n");
			
			OneWire_fReset();        			// reset the 1-Wire device  
			OneWire_WriteByte(0xCC); 			// "skip ROM" command  
			OneWire_WriteByte(0x44);			// "read scratchpad" command 
		}
	}
}

// function for the lcd output
void lcdOutput(int yInt,int xInt,int contentInt){
	itoa(intToString,contentInt,10);
	LCD_Position(yInt,xInt);
	LCD_PrString(intToString);
}

// function to clear the lcd
void lcdClean(void){
	LCD_Position(0,0);
	LCD_PrCString("                ");
	LCD_Position(1,0);
	LCD_PrCString("                ");
}


void analogValueFunc(void){
	valueAnalog = stemp/ntemp;	// calculate average of 50 data
	ntemp = 0;			// set the number and addition to zero
	stemp = 0;
	valueAnalog = (valueAnalog*400)/(4096);	// calculate the temperature in celsius
						// T = (4V * Value)/(4096 * 0,01V)
						//4096 couse of the 12 bit by the ADC
	readOneWire++;				//increment readOneWire
}

void oneWireValue(void){
	OneWire_fReset();        		// reset the 1-Wire device  
	OneWire_WriteByte(0xCC); 		// "skip ROM" command  
	OneWire_WriteByte(0xBE); 		// "convert temperature" command  
	valueOneWire = (OneWire_bReadByte())/2;  // read temeperature and send to valueOneWire
}

void i2cValue(void){
	I2Cm_fSendStart(0x68,I2Cm_WRITE);    	// Do a write
	I2Cm_fWrite(0x00);                   	// the slave acknowledged the mast	//Set sub address to zero

	I2Cm_fSendRepeatStart(0x68,I2Cm_READ);  // Do a read
	valueI2C = I2Cm_bRead(I2Cm_NAKslave);   // Read data byte and NAK the slave to signify end of read.
	I2Cm_SendStop();
}

void spiValue(void ){
	
	SPIM_SendTxData(0xff);						// by sending data the receive starts
	while (!(SPIM_bReadStatus() & SPIM_SPIM_RX_BUFFER_FULL));	// check if the read buffer is full
	cRXDataSPI = SPIM_bReadRxData();				// read the data in the buffer 8 bit at a time
					
	SPIM_SendTxData(0xff);	// again reading 8 bits because the slave sends 16 bits
				
	while (!(SPIM_bReadStatus() & SPIM_SPIM_RX_BUFFER_FULL));
	cRXData1SPI = SPIM_bReadRxData();		// reading 2nd byte
	togetherSPI =  cRXDataSPI + cRXData1SPI;	// putting all bits together
	togetherSPI = togetherSPI >> 1;			// ADC has just 12 bits. LSB must be out shifted
	valueSPI = togetherSPI & maskSPI; 		// first 4 bits (MSB) will be set to 0
	longValueSPI = valueSPI * 409;			// calculating the value of adc
	valueSPI = longValueSPI / 4096; 		// final result
}

//called when ADC conversion is ready
void CalcMean(void){
	ntemp++;
	stemp += EzADC_iGetDataClearFlag(); 
}
