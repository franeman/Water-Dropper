/*
 * Ethan Grey
 * This program will drop a droplet through a solenoid valve and take a picture of the splash
 * The program assumes constance zero acceleration and neglects air resistance
 */

/*
LCD Circuit:
LCD RS pin to digital pin 12
LCD Enable pin to digital pin 11
LCD D4 pin to digital pin 5
LCD D5 pin to digital pin 4
LCD D6 pin to digital pin 3
LCD D7 pin to digital pin 2
Additionally, wire a 10k pot to +5V and GND, with it's wiper (output) to LCD screens VO pin (pin3).
A 220 ohm resistor is used to power the back light of the display, usually on pin 15 and 16 of the LCD connector
*/

/*Beginning of Auto generated code by Atmel studio */
#include <Arduino.h>

/*End of auto generated code by Atmel studio */

#include <LiquidCrystal.h> // Used to interface with LCD
#include <IRremote.h> // Used to read IR remote signal
//Beginning of Auto generated function prototypes by Atmel Studio
//End of Auto generated function prototypes by Atmel Studio

////////////////////////////////////////////////////////////////
// Function prototypes
////////////////////////////////////////////////////////////////
unsigned long calcTTT(float distTarget);
void shoot();
short readRemote();
void getLCDInput();
void clearChar(char* array, short pos, short length);
String charToString(char* array, short length);
void empty(char* array, short length);
short charToShort(char* array, short length);
float charToFloat(char* array, short length);
short charToNum(char character);
bool checkParameters();
void printError();
void insert(char* array, short length, short pos, char charToInsert);
unsigned long microToMilli(unsigned long num);
void dropWater(short dropletSize); // Drop the droplet based on time open in ms
void dropWater(short dropletSize, short numDrops, short timeBetween);


// Pin numbers
// Inputs
#define REMOTE 8
// Outputs
#define CAM 13
// Solenoid relay on A0

bool fire = false; // Determines if the dropper is in dropping or options mode
short menu = 0; // Used for determining which menu page to show
unsigned long timeToTarget = 0; // Time till the droplet hits the target
float distTarget = 0; // Distance from the end of the nozzel to the target in inches
short numDrops = 1; // Number of drops
short dropletSize = 0; // Size of droplets
short timeBetween = 0; // Time between droplets

bool valveOpen = false;
unsigned long closeTime = 0;

// IR remote
IRrecv irrecv(REMOTE);
decode_results results;
short remIn = -1;

// LCD
#define rs 12
#define en 11
#define d4 5
#define d5 4
#define d6 3
#define d7 2

// Character arrays for storing LCD input
#define SIZE 4 // Number of digits per input
#define INPUT_OFFSET (16-SIZE)
char lcdIn0[SIZE]; // Dist to target
char lcdIn1[SIZE]; // Droplet Size
char lcdIn2[SIZE]; // Number of droplets
char lcdIn3[SIZE]; // Time between drops (ms)

short row = 0; // used to select the row of LCD display
short col = 0; // used to select the column of LCD display

LiquidCrystal lcd(rs,en,d4,d5,d6,d7); // create LiquidCrystal variable using specified pins

void setup() {
	// Inputs
	pinMode(REMOTE,INPUT);
	
	// Outputs
	pinMode(CAM,OUTPUT);
	DDRC |= 0x01; // Set ADC0 (A0) to be a digital output
	
	// IR remote
	irrecv.enableIRIn(); // Start the receiver
	
	// LCD
	lcd.begin(16,2); // columns,rows
	
	lcd.home();
	lcd.print("Water Dropper");
	lcd.setCursor(0,1);
	lcd.print("By Ethan Grey");
	delay(5000);
	
	empty(lcdIn0,SIZE); // Fill input arrays with spaces
	empty(lcdIn1,SIZE);
	empty(lcdIn2,SIZE);
	empty(lcdIn3,SIZE);
	
	lcdIn1[1] = '1'; // Set initial droplet size to 100
	lcdIn1[2] = '0'; 
	lcdIn1[3] = '0'; 
	
	lcdIn2[3] = '1'; // Set initial numDrops to 1
	lcdIn3[3] = '0'; // Set initial time between drops to 0
	
	Serial.begin(9600); // Open serial monitor (For debugging)
}

void loop() {
	menu = 0;
	while(!fire)  // Enter options while not ready to fire
	{
	// reset input cursor pos
	row = 0;
	col = 15;
	
	/////////////////////////////////////
	// Menu 0
	///////////////////////////////////// 
	while (menu == 0 && !fire) // Second page of menu, set distTarget and distTrig1
	{
		lcd.clear();
		lcd.cursor(); // Show cursor
		lcd.home();
		String tmpStr = charToString(lcdIn0,SIZE);
		lcd.print("Dist Target:");
		lcd.print(tmpStr);
		lcd.setCursor(0,1);
		tmpStr = charToString(lcdIn1,SIZE);
		lcd.print("Drop Size:");
		lcd.print(tmpStr);
		lcd.setCursor(col,row); // Set cursor to user input
		getLCDInput(); // Handles user input to LCD)
		delay(500);
	}
	// reset input cursor pos
	row = 0;
	col = 15;
	
	/////////////////////////////////////
	// Menu 1
	/////////////////////////////////////
	 
	while (menu == 1 && !fire) // Third page of menu, set trigger val
	{
		lcd.clear();
		lcd.cursor(); // Show cursor
		lcd.home();
		String tmpStr = charToString(lcdIn2,SIZE);
		lcd.print("Num Drops:");
		lcd.print(tmpStr);
		lcd.setCursor(0,1);
		lcd.print("Time Diff:");
		tmpStr = charToString(lcdIn3,SIZE);
		lcd.print(tmpStr);
		lcd.setCursor(col,row); // Set cursor to user input
		getLCDInput(); // Handles user input to LCD)
		delay(500);
	}
	}
 
	/////////////////////////////////////
	// Fire mode
	/////////////////////////////////////
	lcd.clear();
	lcd.home();
	lcd.print("Dropping...");
 
	dropWater(dropletSize,numDrops,timeBetween); // Drop the drops

}

unsigned long calcTTT(float distTarget) // Takes distance between the muzzle to the target in inches and calculates the time left till impact in microseconds                             
  {
    float timeSec =  sqrt((2*(distTarget*12))/32.2); // t(sec) = sqrt((2 * dist(ft)) / 32.2 ft/s^2)
    unsigned long timeToTarget = (unsigned long)timeSec * (unsigned long)pow(10,6); // Convert sec to microSec
    return timeToTarget;
  }
  
void shoot() // Triggers the flash
  {
    
    PORTB |= (1<<5); // fire flash
    delay(500);
    PORTB &= ~(1<<5);
  }
  
short readRemote()
  {
    short input = -1; // -1 indicates a value we weren't looking for
    
    if (irrecv.decode(&results)) // if we relive an IR signal
    {
     Serial.println(results.value, HEX);
     switch (results.value) // Check what button was pressed
     {
       case 0xFF6897: // 0
       input = 0;
         break;
       case 0xFF30CF: // 1
       input = 1;
         break;
       case 0xFF18E7: // 2
       input = 2;
         break;
       case 0xFF7A85: // 3
       input = 3;
         break;
       case 0xFF10EF: // 4
       input = 4;
         break;
       case 0xFF38C7: // 5
       input = 5;
         break;
       case 0xFF5AA5: // 6
       input = 6;
         break;
       case 0xFF42BD: // 7
       input = 7;
         break;
       case 0xFF4AB5: // 8
       input = 8;
         break;
       case 0xFF52AD: // 9
       input = 9;
         break;
       case 0xFF906F: // EQ
       input = 10;
         break;
       case 0xFF22DD: // Prev
       input = 11;
         break;
       case 0xFF02FD: // Next
       input = 12;
         break;
       case 0xFF629D: // CH
       input = 13;
         break;
       case 0xFFE01F: // -
       input = 14;
         break;
       case 0xFFA857: // +
       input = 15;
         break;
       case 0xFF9867: // 100+
       input = 16;
         break;
       case 0xFFA25D: // CH-
       input = 17;
         break;
       case 0xFFE21D: // CH+
       input = 18;
         break;
       case 0xFFC23D: // Play/Pause
       input = 19;
         break;
       case 0xFFB04F:
       input = 20;
         break;
     }
     
     irrecv.resume(); // Receive the next value
    }
     return input; // Return the button that was pressed
  }
  
void getLCDInput() // Handles LCD Input
  {
    short val = readRemote();
    Serial.println("val = " + val);
    switch (val)
          {
            case 0:
              if (menu == 1)
                {
                  switch (row)
                    {
                      case 0:
                        insert(lcdIn0, SIZE, col - INPUT_OFFSET, '0');
                        break;
                      case 1:
                        insert(lcdIn1, SIZE, col - INPUT_OFFSET, '0');
                        break;
                    }
                }
                  else
                    switch (row)
                    {
                      case 0:
                        insert(lcdIn2, SIZE, col - INPUT_OFFSET, '0');
                        break;
                      case 1:
                        insert(lcdIn3, SIZE, col - INPUT_OFFSET, '0');
                        break;
                    }
                
              break;
            case 1:
              if (menu == 1)
                {
                  switch (row)
                    {
                      case 0:
                        insert(lcdIn0, SIZE, col - INPUT_OFFSET, '1');
                        break;
                      case 1:
                        insert(lcdIn1, SIZE, col - INPUT_OFFSET, '1');
                        break;
                    }
                }
              else
                {
                  switch (row)
                    {
                      case 0:
                        insert(lcdIn2, SIZE, col - INPUT_OFFSET, '1');
                        break;
                      case 1:
                        insert(lcdIn3, SIZE, col - INPUT_OFFSET, '1');
                        break;
                    }
                }
              break;
            case 2:
              if (menu == 1)
                {                
                  switch (row)
                    {
                      case 0:
                        insert(lcdIn0, SIZE, col - INPUT_OFFSET, '2');
                        break;
                      case 1:
                        insert(lcdIn1, SIZE, col - INPUT_OFFSET, '2');
                        break;
                    }
                }
              else
                {
                  switch (row)
                    {
                      case 0:
                        insert(lcdIn2, SIZE, col - INPUT_OFFSET, '2');
                        break;
                      case 1:
                        insert(lcdIn3, SIZE, col - INPUT_OFFSET, '2');
                        break;
                    }
                }
              break;
            case 3:
              if (menu == 1)
                {
                  switch (row)
                    {
                      case 0:
                        insert(lcdIn0, SIZE, col - INPUT_OFFSET, '3');
                        break;
                      case 1:
                        insert(lcdIn1, SIZE, col - INPUT_OFFSET, '3');
                        break;
                    }
                }
              else
                {
                  switch (row)
                    {
                      case 0:
                        insert(lcdIn2, SIZE, col - INPUT_OFFSET, '3');
                        break;
                      case 1:
                        insert(lcdIn3, SIZE, col - INPUT_OFFSET, '3');
                        break;
                    }
                }
              break;
            case 4:
              if (menu == 1)
              {
                switch (row)
                  {
                    case 0:
                      insert(lcdIn0, SIZE, col - INPUT_OFFSET, '4');
                      break;
                    case 1:
                      insert(lcdIn1, SIZE, col - INPUT_OFFSET, '4');
                      break;
                  }
              }
            else
              {
                switch (row)
                    {
                      case 0:
                        insert(lcdIn2, SIZE, col - INPUT_OFFSET, '4');
                        break;
                      case 1:
                        insert(lcdIn3, SIZE, col - INPUT_OFFSET, '4');
                        break;
                    }
              }
              break;
            case 5:
              if (menu == 1)
                {
                  switch (row)
                    {
                      case 0:
                        insert(lcdIn0, SIZE, col - INPUT_OFFSET, '5');
                        break;
                      case 1:
                        insert(lcdIn1, SIZE, col - INPUT_OFFSET, '5');
                        break;
                    }
                }
              else
                {
                  switch (row)
                    {
                      case 0:
                        insert(lcdIn2, SIZE, col - INPUT_OFFSET, '5');
                        break;
                      case 1:
                        insert(lcdIn3, SIZE, col - INPUT_OFFSET, '5');
                        break;
                    }
                }
              break;
            case 6:
              if (menu == 1)
                {
                  switch (row)
                    {
                      case 0:
                        insert(lcdIn0, SIZE, col - INPUT_OFFSET, '6');
                        break;
                      case 1:
                        insert(lcdIn1, SIZE, col - INPUT_OFFSET, '6');
                        break;
                    }
                }
              else
                {
                  switch (row)
                    {
                      case 0:
                        insert(lcdIn2, SIZE, col - INPUT_OFFSET, '6');
                        break;
                      case 1:
                        insert(lcdIn3, SIZE, col - INPUT_OFFSET, '6');
                        break;
                    }
                }
              break;
            case 7:
              if (menu == 1)
                {
                  switch (row)
                    {
                      case 0:
                        insert(lcdIn0, SIZE, col - INPUT_OFFSET, '7');
                        break;
                      case 1:
                        insert(lcdIn1, SIZE, col - INPUT_OFFSET, '7');
                        break;
                    }
                }
              else
                {
                  switch (row)
                    {
                      case 0:
                        insert(lcdIn2, SIZE, col - INPUT_OFFSET, '7');
                        break;
                      case 1:
                        insert(lcdIn3, SIZE, col - INPUT_OFFSET, '7');
                        break;
                    }
                }
              break;
            case 8:
              if (menu == 1)
                {
                  switch (row)
                    {
                      case 0:
                        insert(lcdIn0, SIZE, col - INPUT_OFFSET, '8');
                        break;
                      case 1:
                        insert(lcdIn1, SIZE, col - INPUT_OFFSET, '8');
                        break;
                    }
                }
              else
                {
                  switch (row)
                    {
                      case 0:
                        insert(lcdIn2, SIZE, col - INPUT_OFFSET, '8');
                        break;
                      case 1:
                        insert(lcdIn3, SIZE, col - INPUT_OFFSET, '8');
                        break;
                    }
                }
              break;
            case 9:
              if (menu == 1)
                {
                  switch (row)
                    {
                      case 0:
                        insert(lcdIn0, SIZE, col - INPUT_OFFSET, '9');
                        break;
                      case 1:
                        insert(lcdIn1, SIZE, col - INPUT_OFFSET, '9');
                        break;
                    }
                }
              else
                {
                  switch (row)
                    {
                      case 0:
                        insert(lcdIn2, SIZE, col - INPUT_OFFSET, '9');
                        break;
                      case 1:
                        insert(lcdIn3, SIZE, col - INPUT_OFFSET, '9');
                        break;
                    }
                }
              break;
            case 11: // Prev
              menu--;
              break;
            case 12: // Next
              menu++; 
              break;
            case 13: // CH
              if (menu == 1)
                {
                  switch (row)
                    {
                      case 0:
                        clearChar(lcdIn0, col - INPUT_OFFSET, SIZE);
                        break;
                      case 1:
                        clearChar(lcdIn1, col - INPUT_OFFSET, SIZE);
                        break;
                    }
                }
              else
                {
                  switch (row)
                    {
                      case 0:
                        clearChar(lcdIn2, col - INPUT_OFFSET, SIZE);
                        break;
                      case 1:
                        clearChar(lcdIn3, col - INPUT_OFFSET, SIZE);
                        break;
                    }
                }
              break;
            /*
            case 14: // - (move cursor left)
              if (col > 12 ) // Make sure user dosn't pass input area
                {
                  col--;
                }
              break;
            case 15: // + (move cursor right)
              if (col < 15) // Make sure user dosn't pass input area
                {
                  col++;
                }
              break;
            */ // Removed changing cursor position
            case 16: // 100+ (add decimal point)
              if (menu == 1)
                {
                  switch (row)
                    {
                      case 0:
                        insert(lcdIn0, SIZE, col - INPUT_OFFSET, '.');
                        break;
                    }
                }
              break;
            case 17: // CH-  (move cursor down)
              if (row == 0) // Make sure user doesn't pass input area
                {
                  row++;
                  col = 15;
                }
              break;
            case 18: // CH+ (move cursor up)
              if (row == 1) // Make sure user doesn't pass input area
                {
                  row--;
                  col = 15;
                }
              break;  
            case 19: // Play/Pause (enter fire mode)
              if (checkParameters()) // If we have all needed variables
                {
                  fire = true; // Set fire mode
                }          
              else // We don't have needed variables
                {
                  printError();
                }
                break;
            case 20:
              if (menu == 2 && row == 1)
                {
                  insert(lcdIn3, SIZE, col - INPUT_OFFSET, '-');
                }
          }
  }
  
void clearChar(char array[], short pos, short length)
    {
       for (short c = pos; c >= 0; c--)
         {
           if (c != 0) // if c isn't the first value in the array
             {
               array[c] = array[c-1]; // copy the value to the left of the array
             }
           else // c is the first value in the array
             {
               array[c] = ' '; // clear the value
             }
         }
       
    }

void empty (char array[], short length)
  {
    for (int c = 0; c < length; c++)
      {
        array[c] = ' ';
      }
  }

short charToShort (char array[], short length)
  {
    bool negative = false;
    bool hasVal = false; // used to determine if we have a value
    bool error = false;
    short num = 0;
    short results = 0;
    for (short c = 0; c < length; c++)
      {
        results = charToNum(array[c]);
        if (results == -1 || results == 10) // If char is a not digit or is a .
          {
            Serial.print("results: ");
            Serial.println(results);
            
            Serial.print("hasVal: ");
            Serial.println(hasVal);
            
            //Serial.print("finished: ");
            //Serial.println(finished);
            
            error = true;
          }
        else if (results == 11) // If it is a space
          {
            // Do nothing
          }
        else if (results == 20) // If it is a -
          {
            negative = true;
          }
        else
          {
            num = num + (results * pow(10,(length - 1) - c)); // add the number to the sum
            hasVal = true; // We have at least one value
            Serial.println("hasVal = true");
          }
      }
    if (!error && hasVal) // If there was no error
      {
        if (negative)
          {
            num = num * -1;
          }
        return num; // Return the number
      }
    else
      {
        return -1; // return error code
      }
  }
  
float charToFloat (char array[], short length)
  {
    bool hasVal = false; // used to determine if we have a value
    bool error = false;
    bool decimal = false;
    short decimalPos = 0;
    float num = 0;
    short results = 0;
    
    for (short c = 0; c < length; c++)
      {
        results = charToNum(array[c]);
        if ((results == -1) || (results == 10 && decimal)) // If char is a not digit, another . is recived
          {
            Serial.print("results: ");
            Serial.println(results);
            
            Serial.print("hasVal: ");
            Serial.println(hasVal);
            
            //Serial.print("finished: ");
            //Serial.println(finished);
            
            error = true;
          }
        else if ((results == 10)) // if we get a .
          {
            decimal = true;
            decimalPos = c;
          }
        else if (decimal) // If we have recived a . and are not done
          {
            num = num + (results * pow(10,-1 *(c - decimalPos))); // add the number to the sum
            hasVal = true; // We have at least one digit
            Serial.println("hasVal = true");
          }
        else if (results == 11) // We received a space
          {
            // Do nothing
          }
        else
          {
            if (c !=0)
            {
              num = num * 10 + results; // add the number to the sum
            }
            else
            {
              num = num + results;
            }
            hasVal = true; // We have at least one digit 
            Serial.println("hasVal = true");
          }
      }
    if (!error && hasVal) // If there was no error and we had a value
      {
        return num; // Return the number
      }
    else
      {
        return -1; // return error code
      }
  }

String charToString (char array[], short length)
{
	String str;
	for (short c = 0; c < length; c++)
	{
		str = str + array[c];
	}
	return str;
	
}

short charToNum(char character)
  // Converts character to digit or . 
  // -1 indicates neither
  // 10 indicates .
  // 11 indicates ' '
  {
    short num = -1; // Set num to -1 to indicate not a number
    if ((character >= 48 && character <= 57) || character == 46 || character == ' ' || character == '-') // If character is a digit, a . , or ' '
      {
        if (character == 46)
          {
            num = 10;
          }
        else if (character == ' ')
          {
            num = 11;
          }
        else if (character == '-')
          {
            num = 20;
          }
        else 
          {
            num = character - 48; // Convert ASCII to digit
          }
      }
    else // character is not a digit or a .
      {
        num = -1;
        Serial.print(character);
        Serial.println(" is not a number, . , or ' '");
      }
    Serial.print("num: ");
    Serial.println(num);
    return num;
  }
  
bool checkParameters()
  {
    Serial.println("Start check");
    bool params = true; // have all parameters default
    // temp values so the globals aren't affected
    float distTar = charToFloat(lcdIn0,SIZE);
    float dropSize = charToShort(lcdIn1,SIZE);
    short numDrop = charToShort(lcdIn2,SIZE);
    short timeBet = charToShort(lcdIn3,SIZE); 
 
    //short trigHigh = 1024; // Defualt value is high to prevent missfires
    //short trim = 0; // used as an offset 
    
    //float distTarget = 0;   // Distance between end of muzzle to target in ft
    //float distTrig1 = 0;    // Distance between muzzle and trig1 in ft   
    
    if(distTar != -1) // Check Dist Target
      {
        distTarget = distTar;
      }
    else
      {
        params = false;
      }
      
    if(dropSize != -1) // Check Dist Trig 1
      {
        dropletSize = dropSize;
      }
    else
      {
        params = false;
      }
      
    if (numDrop != -1) // Check Trig Val
      {
        numDrops = numDrop;
      }
    else
      {
        params = false;
      }
      
    if (timeBet != -1) // Check Trim
      {
        timeBetween = timeBet;
      }
    else
      {
        params = false;
      }
    return params;
  }
void printError()
  {
    Serial.println("Start error");
    lcd.clear(); // Clear the lcd
    lcd.noCursor();
    lcd.home(); // Set cursor to home position
    lcd.print("Error in values"); // Print error message
    Serial.println("Start delay");
    delay(3000); // Wait for 3 seconds
    Serial.println("End error");
    return;
  }
  
void insert(char array[], short length, short pos, char charToInsert)
  {
    if (array[0] == ' ') // If the first item in the array is empty (array is not full)
      {
        for (short c = 0; c < length; c++)
          {
            if (c != length - 1) // if we are not at the end of the array
              {
                array[c] = array[c+1]; // copy the character to the right
              }
            else // we are at the end of the array
              {
                array[c] = charToInsert; // insert the character
              } 
          }
      }
    else
      {
        lcd.clear();
        lcd.home();
        lcd.print("Value full!");
        delay(2000);
      }
  }
unsigned long microToMilli(unsigned long num)
  {
    unsigned long result = num * pow(10,-3);
    return result;
  }

void dropWater(short dropletSize) // Drop the droplet based on time open in ms
{
	if(!valveOpen) // If the solenoid isn't open
	{
		PORTC |= 1; // Open solenoid
		closeTime = millis() + dropletSize; // Calculate time to close
		valveOpen = true;
	}
	else if(millis() >= closeTime) // Valve is open and we need to close
	{
		PORTC &= 0; // Close solenoid	
		valveOpen = false;
	}
}

void dropWater(short dropletSize, short numDrops, short timeBetween)
{
	unsigned long impactTime = micros() + calcTTT(distTarget);
	
	if (numDrops == 1) // if we are only dropping 1 drop
	{
		dropWater(dropletSize);
		while (micros() < impactTime)
		{
			// wait
		}	
		shoot(); // Fire the flash
	}
	else if (numDrops > 1) // if we are dropping multiple drops
	{
		short dropped = 0;
		bool takenPic = false; 
		unsigned long lastDrop = 0;
		unsigned long dropInterval = timeBetween * pow(10,-3); // Convert timeBetween to microseconds
		dropWater(dropletSize); // drop first drop
		lastDrop = micros(); // record time of drop
		dropped++; // increment the drop counter
		
		while (dropped < numDrops) // while we haven't dropped all the drops
		{
			if (!takenPic && micros() >= impactTime) // If we haven't taken the picture and we need to
			{
				shoot(); // Fire the flash
			}
			
			if(micros() >= (lastDrop + dropInterval)) // if we need to drop another drop
			{
				dropWater(dropletSize); // Drop the drop
			}
		}
		
	}
}
