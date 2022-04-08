//Copyright 2021-2022(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

//Handles serial debug data transfers from LIBCM to  host
//FYI:    Serial       data transfers from host  to LiBCM are handled elsewhere.

//JTS2doLater: Gather all Serial Monitor transmissions here

#include "libcm.h"

uint8_t line[USER_INPUT_BUFFER_SIZE]; //Stores user text as it's read from serial buffer

/////////////////////////////////////////////////////////////////////////////////////////////

void printStringStoredInArray(const uint8_t *s)
{
  while (*s) { Serial.write(*s++); } //write each character until '0' (EOL character)
}

/////////////////////////////////////////////////////////////////////////////////////////////

void printDebug(void)
{
	Serial.print(F("\nDebug data persists in EEPROM until cleared ('$DEBUG=CLR' to clear)"));

	Serial.print(F("\n -Has LiBCM limited assist since last cleared?: "));
	(EEPROM_hasLibcmDisabledAssist_get() == EEPROM_LICBM_DISABLED_ASSIST) ? Serial.print(F("YES")) : Serial.print(F("NO"));
	
	Serial.print(F("\n -Has LiBCM limited regen since last cleared?: "));
	(EEPROM_hasLibcmDisabledRegen_get() == EEPROM_LICBM_DISABLED_REGEN) ? Serial.print(F("YES")) : Serial.print(F("NO"));

	Serial.print(F("\n -LOOP_RATE_MILLISECONDS exceeded since last cleared?: "));
	(EEPROM_hasLibcmFailedTiming_get() == EEPROM_LIBCM_LOOPRATE_EXCEEDED) ? Serial.print(F("YES")) : Serial.print(F("NO"));
}

/////////////////////////////////////////////////////////////////////////////////////////////

void USB_userInterface_runTestCode(uint8_t testToRun)
{
	//Add whatever code you want to run whenever the user types '$TEST1'/2/3/etc into the Serial Monitor Window
	if     (testToRun == STRING_TERMINATION_CHARACTER) { Serial.print(F("\nError: Test not specified")); }
	else if(testToRun == '1')
	{
		Serial.print(F("\nRunning TEST1"));
		EEPROM_hasLibcmDisabledRegen_set(EEPROM_LICBM_DISABLED_REGEN);
	}
	else if(testToRun == '2')
	{
		Serial.print(F("\nRunning TEST2"));
		EEPROM_hasLibcmDisabledRegen_set(EEPROM_REGEN_NEVER_LIMITED);
	}
	else if(testToRun == '3')
	{
		Serial.print(F("\nRunning TEST3"));
		EEPROM_hasLibcmDisabledAssist_set(EEPROM_LICBM_DISABLED_ASSIST);
	}
	else if(testToRun == '4')
	{
		Serial.print(F("\nRunning TEST4"));
		EEPROM_hasLibcmDisabledAssist_set(EEPROM_ASSIST_NEVER_LIMITED);
	}
	else if(testToRun == '5')
	{
		Serial.print(F("\nRunning TEST5"));

		//load jibberish into EEPROM
		EEPROM_hasLibcmDisabledAssist_set(0xFF);
		EEPROM_hasLibcmDisabledRegen_set(0x00);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

//JTS2doLater: Put this somewhere else
void(* rebootLiBCM) (void) = 0;//declare reset function at address 0

/////////////////////////////////////////////////////////////////////////////////////////////

void printHelp(void)
{
	Serial.print(F("\n\nLiBCM supports the following commands:"
		"\n -'$HELP' display this text."
		"\n -'$BOOT' restart LiBCM."
		"\n -'$TEST_' run test code.  '$TEST1'/2/3/4/etc (see 'USB_userInterface_runTestCode()')"
		"\n -'$DEBUG' display debug info stored in EEPROM.  'DEBUG=CLR' to restore defaults."
		"\n -'$KEYms' display LiBCM keyON delay in ms.  'KEYms=____' to set (0 to 254 ms)"
		"\n"
		/*
		"\nFuture LiBCM commands (not presently supported"
		"\n -'$DEFAULT' restore all EEPROM values to default"
		"\n -'$FAN' display fan status.  '$FAN=OFF'/LOW/HI to set."
		"\n -'$IHACK' display current hack setting.  '$IHACK=00'/20/40/60 to set."
		"\n -'$VHACK' display voltage hack setting.  '$VHACK=OEM'/ASSISTONLY_VAR/ASSISTONLY_BIN/ALWAYS."
		"\n -'$ASSIST_OFF' disable assist until LiBCM resets."
		"\n -'$ASSIST_ON' enable assist until LiBCM resets."
		"\n -'$REGEN_OFF' disable regen until LiBCM resets."
		"\n -'$REGEN_ON' enable regen until LiBCM resets."
		"\n -'MCME_VDELTA' display MCM'E' voltage offset.  'MCME_VDELTA=__' to set."
		"\n -'BATTmAh' display battery capacity in mAh.  'BATTmAh=____' to set."
		"\n -'SoC_MAX' display max allowed SoC.  'SoC_MAX=__' to set."
		"\n -'SoC_MIN' display min allowed SoC.  'SoC_MIN=__' to set."
		"\n -'GRIDVMAX' display max grid charger voltage. 'GRIDVMAX=_.___' to set."
		*/
		));
	//When adding new commands, make sure to add cases to the following functions:
		//USB_userInterface_executeUserInput()
		//EEPROM_resetDebugValues() //if debug data is stored in EEPROM
		//EEPROM_verifyDataValid() //if data is stored in EEPROM
}

/////////////////////////////////////////////////////////////////////////////////////////////

//LiBCM's atoi() implementation for uint8_t
uint8_t get_uint8_FromInput(uint8_t digit1, uint8_t digit2, uint8_t digit3)
{
	bool errorOccurred = false;
	uint8_t numDecimalDigits = 3;
	uint8_t decimalValue = 0;

	if     (digit1 == STRING_TERMINATION_CHARACTER) { errorOccurred = true; }
	else if(digit2 == STRING_TERMINATION_CHARACTER) { numDecimalDigits = 1; }
	else if(digit3 == STRING_TERMINATION_CHARACTER) { numDecimalDigits = 2; }
	
	if(errorOccurred == true) { Serial.print(F("\nInvalid uint8_t Entry")); }
	else
	{
		if     (numDecimalDigits == 1) { decimalValue =                                      (digit1-'0'); }
		else if(numDecimalDigits == 2) { decimalValue =                    (digit1-'0')*10 + (digit2-'0'); }
		else if(numDecimalDigits == 3) { decimalValue = (digit1-'0')*100 + (digit2-'0')*10 + (digit3-'0'); }
	}

	return decimalValue;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void USB_userInterface_executeUserInput(void)
{
	if(line[0] == '$') //valid commands start with '$'
	{
		//$HELP
		if( (line[1] == 'H') && (line[2] == 'E') && (line[3] == 'L') && (line[4] == 'P') ) { printHelp(); }

		//$BOOT
		else if( (line[1] == 'B') && (line[2] == 'O') && (line[3] == 'O') && (line[4] == 'T') )
		{
			Serial.print(F("\nRebooting LiBCM"));
			delay(50); //give serial buffer time to send
			rebootLiBCM();
		}

		//$TEST
		else if( (line[1] == 'T') && (line[2] == 'E') && (line[3] == 'S') && (line[4] == 'T') ) { USB_userInterface_runTestCode(line[5]); }
		
		//$DEBUG
		else if( (line[1] == 'D') && (line[2] == 'E') && (line[3] == 'B') && (line[4] == 'U') && (line[5] == 'G') )
		{ 
			if     ( (line[6] == '=') && (line[7] == 'C') && (line[8] == 'L') && (line[9] == 'R') )
			{ 
				Serial.print(F("\nRestoring default DEBUG values"));
				EEPROM_resetDebugValues();
			}
			else if(line[6] == STRING_TERMINATION_CHARACTER) { printDebug(); }
		}

		//KEYms
		else if( (line[1] == 'K') && (line[2] == 'E') && (line[3] == 'Y') && (line[4] == 'M') && (line[5] == 'S') )
		{
			if (line[6] == '=')
			{
				uint8_t newKeyOnDelay_ms = get_uint8_FromInput(line[7],line[8],line[9]);
				Serial.print("\nnewKeyOnDelay_ms is " + String(newKeyOnDelay_ms) );
				EEPROM_delayKeyON_ms_set(newKeyOnDelay_ms);
			}
			else if(line[6] == STRING_TERMINATION_CHARACTER)
			{
				Serial.print(F("\n Additional delay before LiBCM responds to keyON event (ms): "));
				Serial.print( EEPROM_delayKeyON_ms_get(), DEC);
			}
		}

	

		//$DEFAULT
		else { Serial.print(F("\nInvalid Entry")); }
	}
	else { Serial.print(F("\nInvalid Entry")); }
}

/////////////////////////////////////////////////////////////////////////////////////////////

void USB_userInterface_handler(void)
{
	uint8_t latestCharacterRead = 0; //c
	static uint8_t numCharactersReceived = 0; //char_counter
	static uint8_t inputFlags = 0; //stores state as input text is processed (e.g. whether inside a comment or not)

	while( Serial.available() )
	{
		//user-typed characters are waiting in serial buffer

		latestCharacterRead = Serial.read(); //read next character in buffer
		
		if( (latestCharacterRead == '\n') || (latestCharacterRead == '\r') ) //EOL character retrieved
		{
			//line line is now complete
			line[numCharactersReceived] = STRING_TERMINATION_CHARACTER;

			Serial.print(F("\necho: "));
			printStringStoredInArray(line); //echo user input

			if(numCharactersReceived >= USER_INPUT_BUFFER_SIZE)     { Serial.print(F("\nError: User typed too many characters")); }
			else                                                    { USB_userInterface_executeUserInput();                       }

			numCharactersReceived = 0; //reset for next line
		}
		else //add (non-EOL) character to array 
		{
			if(inputFlags && INPUT_FLAG_INSIDE_COMMENT)
			{
				if(latestCharacterRead == ')') { inputFlags &= ~(INPUT_FLAG_INSIDE_COMMENT); } //end of comment
			}
			else if(numCharactersReceived < (USER_INPUT_BUFFER_SIZE - 1))
			{
				//not presently inside a comment and input buffer not exceeded
				if     (latestCharacterRead == '(') { inputFlags |= INPUT_FLAG_INSIDE_COMMENT;    } //start of comment //ignores all characters until ')'
				else if(latestCharacterRead == ' ') { ;                                           } //throw away whitespaces

				else if( (latestCharacterRead >= 'a') && (latestCharacterRead <= 'z') )
				{
					line[numCharactersReceived++] = latestCharacterRead + ('A' - 'a'); //convert letters to uppercase
				}
				else {line[numCharactersReceived++] = latestCharacterRead; } //store everything else
			}
		}
	}
}