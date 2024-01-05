#include <SPI.h>
/*PINS
   Arduino SPI pins
   MOSI = 11, MISO = 12, SCK = 13, CS = 10
   STM32 SPI pins
   MOSI = PA7, MISO = PA6, SCK = PA5, CS = PA4
*/

uint16_t command = 0b1111111111111111; //read command (0xFFF)

// float timer = 0; //timer for updating the LCD and sending the data through the serial port
// float rpmTimer = 0; //timer for estimating the RPM

class Motor
{
  public:
  
    byte pin;
    double startAngle = 0;
    double totalAngle = 0;
    int turnsCounter = 0;
    int quadrant = 0;

    Motor(byte pinNum)
    {
      pin = pinNum;    
    }

    void init()
    {
      pinMode(pin, OUTPUT); //CS pin - output
      digitalWrite(pin, HIGH);

      startAngle = readRegister();
      turnsCounter = 0;
    }

    double getTotalAngle()
    {
      double degAngle = readRegister(); //read the position of the magnet
      double correctedAngle = correctAngle(degAngle); //normalize the previous reading
      totalAngle = checkQuadrant(correctedAngle); //check the direction of the rotation and calculate the final displacement

      return totalAngle;
    }

    double readRegister()
    {
      uint16_t rawData = 0; //bits from the encoder (16 bits, but top 2 has to be discarded)
      double degAngle = 0; //Angle in degrees
      
      SPI.beginTransaction(SPISettings(3000000, MSBFIRST, SPI_MODE1));

      //--sending the command
      digitalWrite(pin, LOW);
      SPI.transfer16(command);
      digitalWrite(pin, HIGH);

      delay(10);

      //--receiving the reading
      digitalWrite(pin, LOW);
      rawData = SPI.transfer16(command);
      digitalWrite(pin, HIGH);

      SPI.endTransaction();

      rawData = rawData & 0b0011111111111111; //removing the top 2 bits (PAR and EF)

      degAngle = (double)rawData / (double)16384.0 * (double)360.0; //16384 = 2^14, 360 = 360 degrees

      //Serial.print("Deg: ");
      //Serial.println(degAngle);

      return degAngle;
    }

    double correctAngle(float degAngle)
    {
      //recalculate angle
      double correctedAngle = degAngle - startAngle; //this tares the position

      if (correctedAngle < 0) //if the calculated angle is negative, we need to "normalize" it
      {
        correctedAngle = correctedAngle + (double)360.0; //correction for negative numbers (i.e. -15 becomes +345)
      }
      else
      {
        //do nothing
      }
      // Serial.print("Corrected angle: ");
      // Serial.println(correctedAngle, 2); //print the corrected/tared angle

      return correctedAngle;
    }

    double checkQuadrant(double correctedAngle)
    {
      /*
        //Quadrants:
        4  |  1
        ---|---
        3  |  2
      */

      int newQuadrant = 0;

      //Quadrant 1
      if (correctedAngle >= 0 && correctedAngle <= 90)
      {
        newQuadrant = 1;
      }

      //Quadrant 2
      if (correctedAngle > 90 && correctedAngle <= 180)
      {
        newQuadrant = 2;
      }

      //Quadrant 3
      if (correctedAngle > 180 && correctedAngle <= 270)
      {
        newQuadrant = 3;
      }

      //Quadrant 4
      if (correctedAngle > 270 && correctedAngle < 360)
      {
        newQuadrant = 4;
      }
      //Serial.print("Quadrant: ");
      //Serial.println(quadrantNumber); //print our position "quadrant-wise"

      // Serial.print(quadrantNumber);
      // Serial.print(" ");
      // Serial.print(prevQuadrant);
      // Serial.print(" ");

      if (newQuadrant != quadrant) //if we changed quadrant
      {
        // Serial.println("HERE");
        
        if (newQuadrant == 1 && quadrant == 4)
        {
          turnsCounter++; // 4 --> 1 transition: CW rotation
          // rpmCounter++;
        }

        if (newQuadrant == 4 && quadrant == 1)
        {
          turnsCounter--; // 1 --> 4 transition: CCW rotation
          // rpmCounter--;
        }
        //this could be done between every quadrants so one can count every 1/4th of transition

        quadrant = newQuadrant;  //update to the current quadrant

      }
      //Serial.print("Turns: ");
      //Serial.println(numberofTurns); //number of turns in absolute terms (can be negative which indicates CCW turns)

      //after we have the corrected angle and the turns, we can calculate the total absolute position

      // Serial.println(turnsCounter);

      totalAngle = ((double)turnsCounter * (double)360) + correctedAngle; //number of turns (+/-) plus the actual angle within the 0-360 range
      //Serial.print("Total angle: ");
      //Serial.println(totalAngle, 2); //absolute position of the motor expressed in degree angles, 2 digits

      return totalAngle;
    }

};


const byte CS_ROT1 = 31; //Chip select pin for manual switching
const byte CS_ROT2 = 32; //Chip select pin for manual switching
const byte CS_ROT3 = 35; //Chip select pin for manual switching
const byte CS_TRANS1 = 30; //Chip select pin for manual switching
const byte CS_TRANS2 = 33; //Chip select pin for manual switching
const byte CS_TRANS3 = 36; //Chip select pin for manual switching

Motor rot1 = Motor(CS_ROT1);
Motor rot2 = Motor(CS_ROT2);
Motor rot3 = Motor(CS_ROT3);

Motor trans1 = Motor(CS_TRANS1);
Motor trans2 = Motor(CS_TRANS2);
Motor trans3 = Motor(CS_TRANS3);

// uint16_t rawData = 0; //bits from the encoder (16 bits, but top 2 has to be discarded)
// float degAngle = 0; //Angle in degrees
// float startAngle1 = 0; //starting angle for reference
// float startAngle2 = 0; //starting angle for reference
// float startAngle3 = 0; //starting angle for reference
// float correctedAngle = 0; //tared angle value (degAngle-startAngle)
// float totalAngle = 0; //total accumulated angular displacement
// float numberofTurns = 0;
// int rot1Turns = 0;
// int rot2Turns = 0;
// int rot3Turns = 0;
// float rpmCounter = 0; //counts the number of turns for a certain period
// float revsPerMin = 0; //RPM
// int quadrantNumber = 0;
// int rot1Quadrant = 0;
// int rot2Quadrant = 0;
// int rot3Quadrant = 0;


void setup()
{
  SPI.begin();
  Serial.begin(9600);
  Serial.println("AS5048A - Magnetic position encoder");
  // pinMode(CS_ROT1, OUTPUT); //CS pin - output
  // pinMode(CS_ROT2, OUTPUT); //CS pin - output
  // pinMode(CS_ROT3, OUTPUT); //CS pin - output

  // //------------------------------------------------------
  // digitalWrite(CS_ROT1, HIGH);
  // digitalWrite(CS_ROT2, HIGH);
  // digitalWrite(CS_ROT3, HIGH);
  //------------------------------------------------------

  rot1.init();
  rot2.init();
  rot3.init();

  trans1.init();
  trans2.init();
  trans3.init();

  //Checking the initial angle
  // float degAngle = readRegister(CS_ROT1);
  // startAngle1 = degAngle;

  // degAngle = readRegister(CS_ROT2);
  // startAngle2 = degAngle;

  // degAngle = readRegister(CS_ROT3);
  // startAngle3 = degAngle;
}

void loop()
{
  double r1Angle = rot1.getTotalAngle();
  double r2Angle = rot2.getTotalAngle();
  double r3Angle = rot3.getTotalAngle();

  double t1Angle = trans1.getTotalAngle();
  double t2Angle = trans2.getTotalAngle();
  double t3Angle = trans3.getTotalAngle();

  
  if (Serial.available() >= 2)
  {
    int readByte = Serial.read();
    char letter = readByte;

    readByte = Serial.read();
    char number = readByte;

    if(letter == 'r')
    {
      if(number == '1')       Serial.print(r1Angle);
      else if(number == '2')  Serial.print(r2Angle);
      else if(number == '3')  Serial.print(r3Angle);
      else                    Serial.print(number);
    }
    else if (letter == 't')
    {
      if(number == '1')       Serial.print(t1Angle);
      else if(number == '2')  Serial.print(t2Angle);
      else if(number == '3')  Serial.print(t3Angle);
      else                    Serial.print(number);
    }
    else if (letter == 'x' and number == 'x')
    {
      rot1.init();
      rot2.init();
      rot3.init();
      trans1.init();
      trans2.init();
      trans3.init();
      Serial.print((double)0.0);
    }
    else
    {
      Serial.print(letter);
    }
    
    Serial.println();
  }
  
  
  delay(20);
  
  // if (millis() - timer > 50)
  // {
  //   float degAngle = readRegister(CS_ROT1); //read the position of the magnet
  //   float correctedAngle1 = correctAngle(degAngle, startAngle1); //normalize the previous reading
  //   float totalAngle1 = checkQuadrant(correctedAngle1, rot1Quadrant, rot1Turns); //check the direction of the rotation and calculate the final displacement

  //   // Serial.print("Motor 1: ");
  //   // Serial.println(correctedAngle, 2); //print the corrected/tared angle

  //   degAngle = readRegister(CS_ROT2); //read the position of the magnet
  //   float correctedAngle2 = correctAngle(degAngle, startAngle2); //normalize the previous reading
  //   float totalAngle2 = checkQuadrant(correctedAngle2, rot2Quadrant, rot2Turns); //check the direction of the rotation and calculate the final displacement

  //   // Serial.print("Motor 2: ");
  //   // Serial.println(correctedAngle, 2); //print the corrected/tared angle

  //   degAngle = readRegister(CS_ROT3); //read the position of the magnet
  //   float correctedAngle3 = correctAngle(degAngle, startAngle3); //normalize the previous reading
  //   float totalAngle3 = checkQuadrant(correctedAngle3, rot3Quadrant, rot3Turns); //check the direction of the rotation and calculate the final displacement

  //   // Serial.print("Motor 3: ");
  //   // Serial.println(correctedAngle, 2); //print the corrected/tared angle

  //   Serial.print(totalAngle1, 2);
  //   Serial.print(" ");
  //   Serial.print(totalAngle2, 2);
  //   Serial.print(" ");
  //   Serial.println(totalAngle3, 2);

  //   timer = millis();
  // }  
    

  // if (millis() - timer > 250)
  // {
  //   Serial.print("Turns: ");
  //   Serial.println(numberofTurns);

  //   Serial.print("Total Angle: ");
  //   Serial.println(totalAngle, 2);

 
  //   timer = millis();
  // }

  // if (millis() - rpmTimer > 15000) //check and calculate RPM every 15 sec
  // {
  //   revsPerMin = 4 * rpmCounter; //60000/15000 = 4 (we assume the same speed for the whole minute)

  //   rpmCounter = 0; //reset
  //   rpmTimer = millis();
  // }
}

// float readRegister(byte pin)
// {
//   uint16_t rawData = 0; //bits from the encoder (16 bits, but top 2 has to be discarded)
//   float degAngle = 0; //Angle in degrees
  
//   SPI.beginTransaction(SPISettings(3000000, MSBFIRST, SPI_MODE1));

//   //--sending the command
//   digitalWrite(pin, LOW);
//   SPI.transfer16(command);
//   digitalWrite(pin, HIGH);

//   delay(10);

//   //--receiving the reading
//   digitalWrite(pin, LOW);
//   rawData = SPI.transfer16(command);
//   digitalWrite(pin, HIGH);

//   SPI.endTransaction();

//   rawData = rawData & 0b0011111111111111; //removing the top 2 bits (PAR and EF)

//   degAngle = (float)rawData / 16384.0 * 360.0; //16384 = 2^14, 360 = 360 degrees

//   //Serial.print("Deg: ");
//   //Serial.println(degAngle);

//   return degAngle;
// }

// float correctAngle(float degAngle, float startAngle)
// {
//   //recalculate angle
//   float correctedAngle = degAngle - startAngle; //this tares the position

//   if (correctedAngle < 0) //if the calculated angle is negative, we need to "normalize" it
//   {
//     correctedAngle = correctedAngle + 360; //correction for negative numbers (i.e. -15 becomes +345)
//   }
//   else
//   {
//     //do nothing
//   }
//   // Serial.print("Corrected angle: ");
//   // Serial.println(correctedAngle, 2); //print the corrected/tared angle

//   return correctedAngle;
// }

// float checkQuadrant(float correctedAngle, int &prevQuadrant, int &turnsCounter)
// {
//   /*
//     //Quadrants:
//     4  |  1
//     ---|---
//     3  |  2
//   */

//   int quadrantNumber = 0;

//   //Quadrant 1
//   if (correctedAngle >= 0 && correctedAngle <= 90)
//   {
//     quadrantNumber = 1;
//   }

//   //Quadrant 2
//   if (correctedAngle > 90 && correctedAngle <= 180)
//   {
//     quadrantNumber = 2;
//   }

//   //Quadrant 3
//   if (correctedAngle > 180 && correctedAngle <= 270)
//   {
//     quadrantNumber = 3;
//   }

//   //Quadrant 4
//   if (correctedAngle > 270 && correctedAngle < 360)
//   {
//     quadrantNumber = 4;
//   }
//   //Serial.print("Quadrant: ");
//   //Serial.println(quadrantNumber); //print our position "quadrant-wise"

//   // Serial.print(quadrantNumber);
//   // Serial.print(" ");
//   // Serial.print(prevQuadrant);
//   // Serial.print(" ");

//   if (quadrantNumber != prevQuadrant) //if we changed quadrant
//   {
//     // Serial.println("HERE");
    
//     if (quadrantNumber == 1 && prevQuadrant == 4)
//     {
//       turnsCounter++; // 4 --> 1 transition: CW rotation
//       // rpmCounter++;
//     }

//     if (quadrantNumber == 4 && prevQuadrant == 1)
//     {
//       turnsCounter--; // 1 --> 4 transition: CCW rotation
//       // rpmCounter--;
//     }
//     //this could be done between every quadrants so one can count every 1/4th of transition

//     prevQuadrant = quadrantNumber;  //update to the current quadrant

//   }
//   //Serial.print("Turns: ");
//   //Serial.println(numberofTurns); //number of turns in absolute terms (can be negative which indicates CCW turns)

//   //after we have the corrected angle and the turns, we can calculate the total absolute position

//   // Serial.println(turnsCounter);

//   totalAngle = (turnsCounter * 360) + correctedAngle; //number of turns (+/-) plus the actual angle within the 0-360 range
//   //Serial.print("Total angle: ");
//   //Serial.println(totalAngle, 2); //absolute position of the motor expressed in degree angles, 2 digits

//   return totalAngle;
// }
