//MVP PoC:

/*
* Don't care about:
* -SoC (accumulation)
* -Temperature
* -Fan speed
* -Spoofing ConnE
* -Spoofing Vpin
* -Grid charger
*/

#define PIN_BATTCURRENT A0
#define PIN_FANOEM_LOW A1
#define PIN_FANOEM_HI A2
#define PIN_TEMP_YEL A3
#define PIN_TEMP_GRN A4
#define PIN_TEMP_WHT A5
#define PIN_TEMP_BLU A6
#define PIN_VPIN_IN A7
#define PIN_TURNOFFLiBCM A8
#define PIN_DEBUG_FET A9
#define PIN_DEBUG_IO1 A10
#define PIN_DEBUG_IO2 A11
#define PIN_LED1 A12
#define PIN_LED2 A13
#define PIN_LED3 A14
#define PIN_LED4 A15

#define PIN_BATTSCI_DIR 2
#define PIN_METSCI_DIR 3
#define PIN_VPIN_OUT 4
#define PIN_SPI_EXT_CS 5 
#define PIN_TEMP_EN 6
#define PIN_CONNE_PWM 7
#define PIN_GRID_PWM 8
#define PIN_GRID_SENSE 9
#define PIN_GRID_EN 10
#define PIN_FAN_PWM 11
#define PIN_LOAD5V 12
#define PIN_KEY_ON 13

//Serial3
#define HLINE_TX 14
#define HLINE_RX 15

//Serial2
#define METSCI_TX 16
#define METSCI_RX 17

//Serial1
#define BATTSCI_TX 18
#define BATTSCI_RX 19

#define DEBUG_SDA 20
#define DEBUG_CLK 21

void setup()
{
	//Prevent LiBCM from turning off the 12V->5V DCDC
	pinMode(PIN_TURNOFFLiBCM,OUTPUT);
	digitalWrite(PIN_TURNOFFLiBCM,LOW);

	//Place constant load on 5V rail to ensure OEM Current Sensor doesn't sink more current than 12V->5V is sourcing 
	pinMode(PIN_LOAD5V,OUTPUT);
	digitalWrite(PIN_LOAD5V,HIGH);

	pinMode(PIN_LED1,OUTPUT);
	digitalWrite(PIN_LED1,HIGH);
	pinMode(PIN_LED2,OUTPUT);
	pinMode(PIN_LED3,OUTPUT);
	pinMode(PIN_LED4,OUTPUT);

	pinMode(PIN_CONNE_PWM,OUTPUT);
	pinMode(PIN_FAN_PWM,OUTPUT);
	pinMode(PIN_FANOEM_LOW,OUTPUT);
	pinMode(PIN_FANOEM_HI,OUTPUT);
	pinMode(PIN_GRID_EN,OUTPUT);
	pinMode(PIN_VPIN_OUT,OUTPUT);

	analogReference(EXTERNAL); //use 5V AREF pin, which is coupled to filtered VCC

	Serial.begin(115200);	//USB
	METSCI_begin();

  //Place into BATTSCI_end()
  pinMode(PIN_BATTSCI_DIR,OUTPUT);
  digitalWrite(PIN_BATTSCI_DIR,LOW); //BATTSCI Set HI to send. Must be low when key OFF to prevent backdriving MCM
  
	Serial.println("Welcome to LiBCM");
}


void loop()
{
	//Enable BATTSCI only when key is on (to prevent backdriving unpowered MCM)
	if( digitalRead(PIN_KEY_ON) ) { BATTSCI_begin(); }
	else { BATTSCI_end(); }

	//Read cell voltages, sum to stack voltage
	//ADD LTC6804 stuff here
	//sum all 48 cells

	//get current sensor value
	int16_t battCurrent_RawADC = analogRead(PIN_BATTCURRENT);
	Serial.print("\nRaw ADC Current Reading is: " + String(battCurrent_RawADC) );

	//convert result into amps
	int16_t battCurrent_amps = ( (battCurrent_RawADC * 13) >> 6) - 68; //Accurate to within 3.7 amps of actual value
	Serial.print(" counts, which is : " + String(battCurrent_amps) + " amps.");

	//METSCI Decoding
  uint8_t *METSCI_Frame = METSCI_getFrame();
  Serial.print("\nMETSCI Frame is: " + String(METSCI_Frame[0],HEX) + String(METSCI_Frame[1],HEX) + String(METSCI_Frame[2],HEX) ); //Number of assist/regen bars displayed
  //METSCI_Frame[0] is Number of assist/regen bars displayed
  //METSCI_Frame[1] is Packet Data Type
  //METSCI_Frame[2] is Packet Data
  
	
	//BATTSCI creation

  
  delay(200);
}


void BATTSCI_begin()
{
  pinMode(PIN_BATTSCI_DIR, OUTPUT);
  digitalWrite(PIN_BATTSCI_DIR,HIGH);
  Serial2.begin(9600,SERIAL_8E1);
}


void BATTSCI_end()
{
  digitalWrite(PIN_BATTSCI_DIR,LOW);
}
