#include <SPI.h>
/*PINS
   Arduino SPI pins
   MOSI = 11, MISO = 12, SCK = 13, CS = 10
   STM32 SPI pins
   MOSI = PA7, MISO = PA6, SCK = PA5, CS = PA4
*/


const byte CS_pin = 53; //Chip select pin for manual switching

uint16_t rawData = 0; //bits from the encoder (16 bits, but top 2 has to be discarded)
float degAngle = 0; //Angle in degrees
float startAngle = 0; //starting angle for reference
float correctedAngle = 0; //tared angle value (degAngle-startAngle)
float totalAngle = 0; //total accumulated angular displacement
float numberofTurns = 0;
float rpmCounter = 0; //counts the number of turns for a certain period
float revsPerMin = 0; //RPM
int quadrantNumber = 0;
int previousquadrantNumber = 0;

uint16_t command = 0b1111111111111111; //read command (0xFFF)

float timer = 0; //timer for updating the LCD and sending the data through the serial port
float rpmTimer = 0; //timer for estimating the RPM

void setup()
{
  SPI.begin();
  Serial.begin(9600);
  Serial.println("AS5048A - Magnetic position encoder");
  pinMode(CS_pin, OUTPUT); //CS pin - output

  //------------------------------------------------------

  //------------------------------------------------------

  //Checking the initial angle
  readRegister();
  startAngle = degAngle;
}

void loop()
{
  readRegister(); //read the position of the magnet
  correctAngle(); //normalize the previous reading
  checkQuadrant(); //check the direction of the rotation and calculate the final displacement

  if (millis() - timer > 250)
  {
    Serial.print("Turns: ");
    Serial.println(numberofTurns);

    Serial.print("Total Angle: ");
    Serial.println(totalAngle, 2);

 
    timer = millis();
  }

  if (millis() - rpmTimer > 15000) //check and calculate RPM every 15 sec
  {
    revsPerMin = 4 * rpmCounter; //60000/15000 = 4 (we assume the same speed for the whole minute)

    rpmCounter = 0; //reset
    rpmTimer = millis();
  }
}

void readRegister()
{
  SPI.beginTransaction(SPISettings(3000000, MSBFIRST, SPI_MODE1));

  //--sending the command
  digitalWrite(CS_pin, LOW);
  SPI.transfer16(command);
  digitalWrite(CS_pin, HIGH);

  delay(10);

  //--receiving the reading
  digitalWrite(CS_pin, LOW);
  rawData = SPI.transfer16(command);
  digitalWrite(CS_pin, HIGH);

  SPI.endTransaction();

  rawData = rawData & 0b0011111111111111; //removing the top 2 bits (PAR and EF)

  degAngle = (float)rawData / 16384.0 * 360.0; //16384 = 2^14, 360 = 360 degrees

  //Serial.print("Deg: ");
  //Serial.println(degAngle);
}

void correctAngle()
{
  //recalculate angle
  correctedAngle = degAngle - startAngle; //this tares the position

  if (correctedAngle < 0) //if the calculated angle is negative, we need to "normalize" it
  {
    correctedAngle = correctedAngle + 360; //correction for negative numbers (i.e. -15 becomes +345)
  }
  else
  {
    //do nothing
  }
  Serial.print("Corrected angle: ");
  Serial.println(correctedAngle, 2); //print the corrected/tared angle
}

void checkQuadrant()
{
  /*
    //Quadrants:
    4  |  1
    ---|---
    3  |  2
  */

  //Quadrant 1
  if (correctedAngle >= 0 && correctedAngle <= 90)
  {
    quadrantNumber = 1;
  }

  //Quadrant 2
  if (correctedAngle > 90 && correctedAngle <= 180)
  {
    quadrantNumber = 2;
  }

  //Quadrant 3
  if (correctedAngle > 180 && correctedAngle <= 270)
  {
    quadrantNumber = 3;
  }

  //Quadrant 4
  if (correctedAngle > 270 && correctedAngle < 360)
  {
    quadrantNumber = 4;
  }
  //Serial.print("Quadrant: ");
  //Serial.println(quadrantNumber); //print our position "quadrant-wise"

  if (quadrantNumber != previousquadrantNumber) //if we changed quadrant
  {
    if (quadrantNumber == 1 && previousquadrantNumber == 4)
    {
      numberofTurns++; // 4 --> 1 transition: CW rotation
      rpmCounter++;
    }

    if (quadrantNumber == 4 && previousquadrantNumber == 1)
    {
      numberofTurns--; // 1 --> 4 transition: CCW rotation
      rpmCounter--;
    }
    //this could be done between every quadrants so one can count every 1/4th of transition

    previousquadrantNumber = quadrantNumber;  //update to the current quadrant

  }
  //Serial.print("Turns: ");
  //Serial.println(numberofTurns); //number of turns in absolute terms (can be negative which indicates CCW turns)

  //after we have the corrected angle and the turns, we can calculate the total absolute position

  totalAngle = (numberofTurns * 360) + correctedAngle; //number of turns (+/-) plus the actual angle within the 0-360 range
  //Serial.print("Total angle: ");
  //Serial.println(totalAngle, 2); //absolute position of the motor expressed in degree angles, 2 digits
}
