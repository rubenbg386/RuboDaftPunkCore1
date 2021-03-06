#include <Wire.h>

#include <Adafruit_NeoPixel.h>
#include <SD.h>
//#include <SdFat.h>
#include "IRremote.h"


#include <LiquidCrystal_I2C.h>

// Pattern types supported:
enum  pattern { NONE, RAINBOW_CYCLE, THEATER_CHASE, COLOR_WIPE, SCANNER, FADE, RUBO };
// Patern directions supported:
enum  direction { FORWARD, REVERSE };

// NeoPattern Class - derived from the Adafruit_NeoPixel class
class NeoPatterns : public Adafruit_NeoPixel
{
public:

	// Member Variables:
	pattern  ActivePattern;  // which pattern is running
	direction Direction;     // direction to run the pattern

	unsigned long Interval;   // milliseconds between updates
	unsigned long lastUpdate; // last update of position

	uint32_t Color1, Color2;  // What colors are in use
	uint16_t TotalSteps;  // total number of steps in the pattern
	uint16_t Index;  // current step within the pattern

	byte Trama[256];	//array de bytes donde se guarda la trama actual

	void (*OnComplete)();  // Callback on completion of pattern

	// Constructor - calls base-class constructor to initialize strip
	NeoPatterns(uint16_t pixels, uint8_t pin, uint8_t type, void (*callback)())
	:Adafruit_NeoPixel(pixels, pin, type)
	{
		OnComplete = callback;
	}

	// Update the pattern
	void Update()
	{
		if((millis() - lastUpdate) > Interval) // time to update
		{
			lastUpdate = millis();
			switch(ActivePattern)
			{
			case RAINBOW_CYCLE:
				RainbowCycleUpdate();
				break;
			case THEATER_CHASE:
				TheaterChaseUpdate();
				break;
			case COLOR_WIPE:
				ColorWipeUpdate();
				break;
			case SCANNER:
				ScannerUpdate();
				break;
			case FADE:
				FadeUpdate();
				break;
			case RUBO:
				RuboUpdate();
				break;
			default:
				break;
			}
		}
	}

	// Increment the Index and reset at the end
	void Increment()
	{
		if (Direction == FORWARD)
		{
			Index++;
			if (Index >= TotalSteps)
			{
				Index = 0;
				if (OnComplete != NULL)
				{
					OnComplete(); // call the comlpetion callback
				}
			}
		}
		else // Direction == REVERSE
		{
			--Index;
			if (Index <= 0)
			{
				Index = TotalSteps-1;
				if (OnComplete != NULL)
				{
					OnComplete(); // call the comlpetion callback
				}
			}
		}
	}

	// Reverse pattern direction
	void Reverse()
	{
		if (Direction == FORWARD)
		{
			Direction = REVERSE;
			Index = TotalSteps-1;
		}
		else
		{
			Direction = FORWARD;
			Index = 0;
		}
	}

	// Initialize for a RuboCycle
	void Rubo(uint32_t color1, uint8_t interval, direction dir = FORWARD)
	{
		ActivePattern = RUBO;
		Interval = interval;
		TotalSteps = 2;
		Color1 = color1;
		Index = 0;
		Direction = dir;
	}

	// Update the Rainbow Cycle Pattern
	void RuboUpdate()
	{
		for(int i = 0; i < 256; i++){
			if(Trama[i]==1){
				//setPixelColor(i, Color(200,200,200));
				setPixelColor(i, Color1);
			}else{
				setPixelColor(i, Color(0,0,0));
			}
		}

		//setPixelColor(20, Color1); // Moderately bright green color.

		show();
		Increment();
	}

	// Initialize for a RainbowCycle
	void RainbowCycle(uint8_t interval, direction dir = FORWARD)
	{
		ActivePattern = RAINBOW_CYCLE;
		Interval = interval;
		TotalSteps = 255;
		Index = 0;
		Direction = dir;
	}

	// Update the Rainbow Cycle Pattern
	void RainbowCycleUpdate()
	{
		for(int i=0; i< numPixels(); i++)
		{
			setPixelColor(i, Wheel(((i * 256 / numPixels()) + Index) & 255));
		}
		show();
		Increment();
	}

	// Initialize for a Theater Chase
	void TheaterChase(uint32_t color1, uint32_t color2, uint8_t interval, direction dir = FORWARD)
	{
		ActivePattern = THEATER_CHASE;
		Interval = interval;
		TotalSteps = numPixels();
		Color1 = color1;
		Color2 = color2;
		Index = 0;
		Direction = dir;
	}

	// Update the Theater Chase Pattern
	void TheaterChaseUpdate()
	{
		for(int i=0; i< numPixels(); i++)
		{
			if ((i + Index) % 3 == 0)
			{
				setPixelColor(i, Color1);
			}
			else
			{
				setPixelColor(i, Color2);
			}
		}
		show();
		Increment();
	}

	// Initialize for a ColorWipe
	void ColorWipe(uint32_t color, uint8_t interval, direction dir = FORWARD)
	{
		ActivePattern = COLOR_WIPE;
		Interval = interval;
		TotalSteps = numPixels();
		Color1 = color;
		Index = 0;
		Direction = dir;
	}

	// Update the Color Wipe Pattern
	void ColorWipeUpdate()
	{
		setPixelColor(Index, Color1);
		show();
		Increment();
	}

	// Initialize for a SCANNNER
	void Scanner(uint32_t color1, uint8_t interval)
	{
		ActivePattern = SCANNER;
		Interval = interval;
		TotalSteps = (numPixels() - 1) * 2;
		Color1 = color1;
		Index = 0;
	}

	// Update the Scanner Pattern
	void ScannerUpdate()
	{
		for (int i = 0; i < numPixels(); i++)
		{
			if (i == Index)  // Scan Pixel to the right
			{
				setPixelColor(i, Color1);
			}
			else if (i == TotalSteps - Index) // Scan Pixel to the left
			{
				setPixelColor(i, Color1);
			}
			else // Fading tail
			{
				setPixelColor(i, DimColor(getPixelColor(i)));
			}
		}
		show();
		Increment();
	}

	// Initialize for a Fade
	void Fade(uint32_t color1, uint32_t color2, uint16_t steps, uint8_t interval, direction dir = FORWARD)
	{
		ActivePattern = FADE;
		Interval = interval;
		TotalSteps = steps;
		Color1 = color1;
		Color2 = color2;
		Index = 0;
		Direction = dir;
	}

	// Update the Fade Pattern
	void FadeUpdate()
	{
		// Calculate linear interpolation between Color1 and Color2
		// Optimise order of operations to minimize truncation error
		uint8_t red = ((Red(Color1) * (TotalSteps - Index)) + (Red(Color2) * Index)) / TotalSteps;
		uint8_t green = ((Green(Color1) * (TotalSteps - Index)) + (Green(Color2) * Index)) / TotalSteps;
		uint8_t blue = ((Blue(Color1) * (TotalSteps - Index)) + (Blue(Color2) * Index)) / TotalSteps;

		ColorSet(Color(red, green, blue));
		show();
		Increment();
	}

	// Calculate 50% dimmed version of a color (used by ScannerUpdate)
	uint32_t DimColor(uint32_t color)
	{
		// Shift R, G and B components one bit to the right
		uint32_t dimColor = Color(Red(color) >> 1, Green(color) >> 1, Blue(color) >> 1);
		return dimColor;
	}

	// Set all pixels to a color (synchronously)
	void ColorSet(uint32_t color)
	{
		for (int i = 0; i < numPixels(); i++)
		{
			setPixelColor(i, color);
		}
		show();
	}

	// Returns the Red component of a 32-bit color
	uint8_t Red(uint32_t color)
	{
		return (color >> 16) & 0xFF;
	}

	// Returns the Green component of a 32-bit color
	uint8_t Green(uint32_t color)
	{
		return (color >> 8) & 0xFF;
	}

	// Returns the Blue component of a 32-bit color
	uint8_t Blue(uint32_t color)
	{
		return color & 0xFF;
	}

	// Input a value 0 to 255 to get a color value.
	// The colours are a transition r - g - b - back to r.
	uint32_t Wheel(byte WheelPos)
	{
		WheelPos = 255 - WheelPos;
		if(WheelPos < 85)
		{
			return Color(255 - WheelPos * 3, 0, WheelPos * 3);
		}
		else if(WheelPos < 170)
		{
			WheelPos -= 85;
			return Color(0, WheelPos * 3, 255 - WheelPos * 3);
		}
		else
		{
			WheelPos -= 170;
			return Color(WheelPos * 3, 255 - WheelPos * 3, 0);
		}
	}
};

void Ring1Complete();
void Ring2Complete();
void StickComplete();
void MatrizPrincipalComplete();

// Define some NeoPatterns for the two rings and the stick
//  as well as some completion routines
/*
NeoPatterns Ring1(24, 5, NEO_GRB + NEO_KHZ800, &Ring1Complete);
NeoPatterns Ring2(16, 6, NEO_GRB + NEO_KHZ800, &Ring2Complete);
NeoPatterns Stick(16, 7, NEO_GRB + NEO_KHZ800, &StickComplete);
 */
NeoPatterns MatrizPrincipal(256, 3, NEO_GRB + NEO_KHZ800, &MatrizPrincipalComplete);

File myFile;
int UltimaPocicion=0;
int totalBytes = 0;
int pausa = 1000;

#define I2C_ADDR    0x27

LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);

#define INITIAL_LOOP_INDEX    2
int loop_index = INITIAL_LOOP_INDEX;

int exitoSD = 0; //0 Fail, 1 Success

int receiver = 11; // Signal Pin of IR receiver to Arduino Digital Pin 11
IRrecv irrecv(receiver);     // create instance of 'irrecv'
decode_results results;      // create instance of 'decode_results'

int index_color = 0;

// Initialize everything and prepare to start
void setup()
{
	//Serial.begin(9600);



	char filename[] = "anim3.txt";

	lcd.begin (16,2);    // Inicializar el display con 16 caraceres 2 lineas

	lcd.setBacklight(HIGH);

	lcd.home ();                   // go home
	lcd.print("Core1.MEGA v1.01");
	lcd.setCursor ( 0, 1 );        // go to the 2nd line
	lcd.print("Start");

	delay(1000);

	lcd.clear();
	lcd.setCursor ( 0, 0 );
	lcd.print("Inicializando IR");
	irrecv.enableIRIn(); // Iniciamos el receptor de infrarrojos

	delay(1000);

	lcd.clear();
	lcd.setCursor ( 0, 0 );
	lcd.print("Inicializando SD");
	//Serial.println("Inicializando SD");

	pinMode(53, OUTPUT);  //make sure that the default chip select pin is set to output
	digitalWrite(53, HIGH);
	delay(1000);

	lcd.clear();

	if (!SD.begin(53)) {

		lcd.setCursor ( 0, 0 );lcd.print("SD Error");
		exitoSD = 0;
	}else{
		lcd.setCursor ( 0, 0 );lcd.print("SD OK");
		exitoSD = 1;
	}


	delay(1000);

	lcd.clear();
	if (SD.exists(filename)) {
		lcd.setCursor ( 0, 0 );lcd.print("Fichero Existe");
	} else {
		lcd.setCursor ( 0, 0 );lcd.print("Fichero NO Existe");
	}
	delay(1000);

	myFile = SD.open(filename);//abrimos  el archivo

	totalBytes=myFile.size();
	lcd.clear();lcd.setCursor (0,0);lcd.print("Totalbytes: ");lcd.print(totalBytes);
	delay(1000);

	/*
   pinMode(8, INPUT_PULLUP);
   pinMode(9, INPUT_PULLUP);

    // Initialize all the pixelStrips
    Ring1.begin();
    Ring2.begin();
    Stick.begin();

    // Kick off a pattern
    Ring1.TheaterChase(Ring1.Color(255,255,0), Ring1.Color(0,0,50), 100);
    Ring2.RainbowCycle(3);
    Ring2.Color1 = Ring1.Color1;
    Stick.Scanner(Ring1.Color(255,0,0), 55);
	 */

	MatrizPrincipal.begin();
	//MatrizPrincipal.Scanner(MatrizPrincipal.Color(255,0,0), 5);
	//MatrizPrincipal.TheaterChase(MatrizPrincipal.Color(255,0,0),MatrizPrincipal.Color(0,0,50),  100);
	//MatrizPrincipal.RainbowCycle(3);
	//MatrizPrincipal.ColorWipe(MatrizPrincipal.Color(255,0,0), 5);
	MatrizPrincipal.Rubo(MatrizPrincipal.Wheel(index_color), 5);

	//inicializamos la trama a encendido
	for(int i = 0; i < 256; i++){
		MatrizPrincipal.Trama[i] = 0;
	}
	//MatrizPrincipal.Color1 = MatrizPrincipal.Wheel(200);
	MatrizPrincipal.Update();

	lcd.clear();lcd.setCursor ( 0, 0 );lcd.print("Entrando en Loop");
	delay(2000);
}

// Main loop
void loop()
{
	/*
	if (irrecv.decode(&results)) { // have we received an IR signal?
		//Serial.println(results.value, HEX);
		lcd.clear();
		lcd.setCursor ( 0, 0 );lcd.print("IR Signal!");
		lcd.setCursor ( 0, 1 );lcd.print(results.value);
		index_color+=10;
		if(index_color>255){index_color=0;}
		MatrizPrincipal.Color1 = MatrizPrincipal.Wheel(index_color);
		irrecv.resume();//volvemos a activar el IR
	}

	*/
	if(exitoSD==1){
		String cadena="";
		if (myFile) {

			if(UltimaPocicion>=totalBytes)   UltimaPocicion=0; //si se llega al final del fichero vuelve al principio

			myFile.seek(UltimaPocicion);

			int charIndex = 0;
			//--Leemos una línea de la hoja de texto--------------
			while (myFile.available()) {

				char caracter=myFile.read();
				charIndex++;

				cadena=cadena+caracter;
				UltimaPocicion=myFile.position();
				if(caracter==10)//ASCII de nueva de línea
						{
					break;
						}
			}
			//---------------------------------------------------
			//myFile.close(); //cerramos el archivo

			//Serial.print("Cadena Leida:");
			//Serial.print(cadena);

			//-----------procesamos la cadena------------
			int index=0;
			char c=cadena[index++];
			pausa=0;
			while (c >= '0' && c <= '9')
			{
				pausa = 10*pausa + (c - '0');
				c = cadena[index++];
			}

			for(int i=0;i<100;i++)
			{
				if(cadena[index+i*2]=='1')
				{
					//enciende led
					MatrizPrincipal.Trama[i]=1;
				}
				else
				{
					//apaga led
					MatrizPrincipal.Trama[i]=0;
				}
			}


		}
	}


	MatrizPrincipal.Update();
	/*
  // Update the rings.
    Ring1.Update();
    Ring2.Update();

    // Switch patterns on a button press:
    if (digitalRead(8) == LOW) // Button #1 pressed
    {
        // Switch Ring1 to FADE pattern
        Ring1.ActivePattern = FADE;
        Ring1.Interval = 20;
        // Speed up the rainbow on Ring2
        Ring2.Interval = 0;
        // Set stick to all red
        Stick.ColorSet(Stick.Color(255, 0, 0));
    }
    else if (digitalRead(9) == LOW) // Button #2 pressed
    {
        // Switch to alternating color wipes on Rings1 and 2
        Ring1.ActivePattern = COLOR_WIPE;
        Ring2.ActivePattern = COLOR_WIPE;
        Ring2.TotalSteps = Ring2.numPixels();
        // And update tbe stick
        Stick.Update();
    }
    else // Back to normal operation
    {
        // Restore all pattern parameters to normal values
        Ring1.ActivePattern = THEATER_CHASE;
        Ring1.Interval = 100;
        Ring2.ActivePattern = RAINBOW_CYCLE;
        Ring2.TotalSteps = 255;
        Ring2.Interval = min(10, Ring2.Interval);
        // And update tbe stick
        Stick.Update();
    }

	 */


}

//------------------------------------------------------------
//Completion Routines - get called on completion of a pattern
//------------------------------------------------------------
/*
// Ring1 Completion Callback
void Ring1Complete()
{
    if (digitalRead(9) == LOW)  // Button #2 pressed
    {
        // Alternate color-wipe patterns with Ring2
        Ring2.Interval = 40;
        Ring1.Color1 = Ring1.Wheel(random(255));
        Ring1.Interval = 20000;
    }
    else  // Retrn to normal
    {
      Ring1.Reverse();
    }
}

// Ring 2 Completion Callback
void Ring2Complete()
{
    if (digitalRead(9) == LOW)  // Button #2 pressed
    {
        // Alternate color-wipe patterns with Ring1
        Ring1.Interval = 20;
        Ring2.Color1 = Ring2.Wheel(random(255));
        Ring2.Interval = 20000;
    }
    else  // Retrn to normal
    {
        Ring2.RainbowCycle(random(0,10));
    }
}

// Stick Completion Callback
void StickComplete()
{
    // Random color change for next scan
    Stick.Color1 = Stick.Wheel(random(255));
}
 */

// MatrizPrincipal Completion Callback
void MatrizPrincipalComplete(){
	MatrizPrincipal.Color1 = MatrizPrincipal.Wheel(random(255));
}
