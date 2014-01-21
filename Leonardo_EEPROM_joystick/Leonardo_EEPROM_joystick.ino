/*
  PROGRAMMABLE JOYSTICK.

Makes a joystick with 1 (digital)stick and 4 buttons into a programmable controller
default values are for the for the game 'Binding of Isaac'.

NOTE: This code will work on Arduino Leonardo and compatible boards, because they are able to act as native 
keyboard and/or mouse devices.

Created 2012
by mark@moore.dk

Feel free use this code and edit, sell, buy, republish-taking-credit or print-out-and-eat it.

http://blog.mememaker.com/2012/binding-of-isaac-joystick/
*/

#include <EEPROM.h> //remembers stuff without power, slow reads, slower writes, limited number of R/W cycles.

//COMBOs: (more than one button pushed within a ~100mS (default value) window eg. both top buttons or both left buttons)

#define COMBO_FILTER_VALUE 100L  //miliseconds to wait for combo
#define MASH_TIMEOUT 3000L       //how long to mash all 4 buttons before board enters programming mode

//let's keep all the joystick info nice and tidy in a little struct:
//IF YOU UNDERSTAND THE STRUCT, YOU UNDERSTAND THE CODE, IF NOT; NOT.

typedef struct //regular keys  
  {
  int EEPROM_ADDR; //associated EEPROM address - where the keybinding is stored
  char key_binding; //actual key-binding, has a default value set in code, but loads from eeprom if available
  int Leonardo_pin; //pin number on the lEO board
  bool active; //is this button currently activated?
  } BINDINGS; 

typedef struct //combos
{
  int EEPROM_ADDR;
  char key_binding;
  bool active;
  bool debounced;       //only used with the MASH COMBO.. it's handled a bit differently because it doesn't send a keypress.
  bool first;           //these booleans are used as sort of a debouncer, to check if the combo was on purpose, or just a result of someone's girlfriend playing tekken...
                        //combined with these 'counters' we get an actual debounce. henceforth known as the tekken-filter.
  long filter_counter;  //variable to store the value of the long millis(); when the two combo buttons are pressed
} COMBO;

typedef struct
  {
  BINDINGS left;            //joystick left
  BINDINGS right;           //joystick right
  BINDINGS up;              //joystick up
  BINDINGS down;            //joystick down
  BINDINGS btn_topLeft;     //top left button
  BINDINGS btn_topRight;    //top right button
  BINDINGS btn_bottomLeft;  //bottom left button
  BINDINGS btn_bottomRight; //bottom right button

  COMBO    C_TOP;           //yup.. press the two buttons at the top for a brand new key.
  COMBO    C_BOTTOM;        //same goes for the bottom. p00t!
  COMBO    C_LEFT;          //lefty losey
  COMBO    C_RIGHT;         //righty tighty

  COMBO    C_MASH;          //ALL FOUR BUTTONS
  } joystick_map;

joystick_map KEYMAP;

void setup() {

/*
 //test dummy write to flash. .. dont overuse this... there's a limited number of flash writes available before it wears out.
  for (int i = 0; i < 256; ++i)
  {
    EEPROM.write(i,255);
  }
*/

  //DEFAULT KEY BINDINGS
  //are set to these values if the EEPROM lookup fails.
  //there are less messy ways to implement this, but for ease of access it's at the top.

  KEYMAP.left.key_binding='a';
  KEYMAP.right.key_binding='d';
  KEYMAP.up.key_binding='w';
  KEYMAP.down.key_binding='s';
  KEYMAP.btn_topLeft.key_binding=KEY_LEFT_ARROW;
  KEYMAP.btn_topRight.key_binding=KEY_UP_ARROW;
  KEYMAP.btn_bottomLeft.key_binding=KEY_DOWN_ARROW;
  KEYMAP.btn_bottomRight.key_binding=KEY_RIGHT_ARROW;
  KEYMAP.C_TOP.key_binding='e';
  KEYMAP.C_BOTTOM.key_binding=32; //Where do astronauts get drunk? ....spacebar. :D
  KEYMAP.C_LEFT.key_binding='r';
  KEYMAP.C_RIGHT.key_binding='q';
  //END OF DEFAULTS.

  //ARDUINO LEONARDO BUTTON MAP - aka. where did you solder the wires to?
  KEYMAP.left.Leonardo_pin=0;
  KEYMAP.right.Leonardo_pin=1;
  KEYMAP.up.Leonardo_pin=2;
  KEYMAP.down.Leonardo_pin=3;
  KEYMAP.btn_topLeft.Leonardo_pin=4;
  KEYMAP.btn_topRight.Leonardo_pin=5;
  KEYMAP.btn_bottomLeft.Leonardo_pin=6;
  KEYMAP.btn_bottomRight.Leonardo_pin=7;
  //END OF LEONARDO PINMAP

  //EEPROM ADDRESSES: - NOT TO BE FUCKED WITH. Go ahead.. try.
  KEYMAP.left.EEPROM_ADDR=0;
  KEYMAP.right.EEPROM_ADDR=1;
  KEYMAP.up.EEPROM_ADDR=2;
  KEYMAP.down.EEPROM_ADDR=3;
  KEYMAP.btn_topLeft.EEPROM_ADDR=4;
  KEYMAP.btn_topRight.EEPROM_ADDR=5;
  KEYMAP.btn_bottomLeft.EEPROM_ADDR=6;
  KEYMAP.btn_bottomRight.EEPROM_ADDR=7;
  KEYMAP.C_TOP.EEPROM_ADDR=8;
  KEYMAP.C_BOTTOM.EEPROM_ADDR=9;
  KEYMAP.C_LEFT.EEPROM_ADDR=10;
  KEYMAP.C_RIGHT.EEPROM_ADDR=11;
  //END OF EEPROM ADDRESS BOOK

  //NOW FOR THE MAGIC:
  //load values from EEPROM... verify that they're not 0xFF (unwritten memory) 
  //and apply either loaded value or default value depending on verification.

//TODO: a more extensive check than just !=0xff .. other values will kill.. check for them.
/*
  byte temp_key_placeholder; //to avoid reading more than once per binding from the PROM

  temp_key_placeholder=(char)EEPROM.read(KEYMAP.left.EEPROM_ADDR);
  if (temp_key_placeholder!=0xff) KEYMAP.left.key_binding=temp_key_placeholder; 
  
  temp_key_placeholder=(char)EEPROM.read(KEYMAP.right.EEPROM_ADDR);
  if (temp_key_placeholder!=0xff) KEYMAP.right.key_binding=temp_key_placeholder; 

  temp_key_placeholder=(char)EEPROM.read(KEYMAP.up.EEPROM_ADDR);
  if (temp_key_placeholder!=0xff) KEYMAP.up.key_binding=temp_key_placeholder; 
    
  temp_key_placeholder=(char)EEPROM.read(KEYMAP.down.EEPROM_ADDR);
  if (temp_key_placeholder!=0xff) KEYMAP.down.key_binding=temp_key_placeholder; 
  
  temp_key_placeholder=(char)EEPROM.read(KEYMAP.btn_topLeft.EEPROM_ADDR);
  if (temp_key_placeholder!=0xff) KEYMAP.btn_topLeft.key_binding=temp_key_placeholder; 
  
  temp_key_placeholder=(char)EEPROM.read(KEYMAP.btn_topRight.EEPROM_ADDR);
  if (temp_key_placeholder!=0xff) KEYMAP.btn_topRight.key_binding=temp_key_placeholder; 

  temp_key_placeholder=(char)EEPROM.read(KEYMAP.btn_bottomLeft.EEPROM_ADDR);
  if (temp_key_placeholder!=0xff) KEYMAP.btn_bottomLeft.key_binding=temp_key_placeholder; 
  
  temp_key_placeholder=(char)EEPROM.read(KEYMAP.btn_bottomRight.EEPROM_ADDR);
  if (temp_key_placeholder!=0xff) KEYMAP.btn_bottomRight.key_binding=temp_key_placeholder;

  temp_key_placeholder=(char)EEPROM.read(KEYMAP.C_TOP.EEPROM_ADDR);
  if (temp_key_placeholder!=0xff) KEYMAP.C_TOP.key_binding=temp_key_placeholder;

  temp_key_placeholder=(char)EEPROM.read(KEYMAP.C_BOTTOM.EEPROM_ADDR);
  if (temp_key_placeholder!=0xff) KEYMAP.C_BOTTOM.key_binding=temp_key_placeholder;

  temp_key_placeholder=(char)EEPROM.read(KEYMAP.C_LEFT.EEPROM_ADDR);
  if (temp_key_placeholder!=0xff) KEYMAP.C_LEFT.key_binding=temp_key_placeholder;

  temp_key_placeholder=(char)EEPROM.read(KEYMAP.C_RIGHT.EEPROM_ADDR);
  if (temp_key_placeholder!=0xff) KEYMAP.C_RIGHT.key_binding=temp_key_placeholder;
*/
  //woosh...  

  // turn on the pullup resistors so it goes high unless connected to ground:
  pinMode(KEYMAP.left.Leonardo_pin, INPUT_PULLUP); //Joystick left
  pinMode(KEYMAP.right.Leonardo_pin, INPUT_PULLUP); //Joystick right
  pinMode(KEYMAP.up.Leonardo_pin, INPUT_PULLUP); //Joystick up
  pinMode(KEYMAP.down.Leonardo_pin, INPUT_PULLUP); //Joystick down
  pinMode(KEYMAP.btn_topLeft.Leonardo_pin, INPUT_PULLUP); //Top left button
  pinMode(KEYMAP.btn_topRight.Leonardo_pin, INPUT_PULLUP); //Top right button
  pinMode(KEYMAP.btn_bottomLeft.Leonardo_pin, INPUT_PULLUP); //Bottom left button
  pinMode(KEYMAP.btn_bottomRight.Leonardo_pin, INPUT_PULLUP); //Bottom right button

  //Keyboard presses
  Keyboard.begin();

  Serial.begin(9600); //oh what a bother... but it has to be done... :/

  delay(5000);

  Serial.println("Goodmorning Human. Shall we play a game?");
  Serial.println("My current configuration is as follows:");
  
  Serial.println("Stick:");
  Serial.println();
  Serial.print("LEFT:");
  Serial.println(KEYMAP.left.key_binding);
  Serial.print("RIGHT");
  Serial.println(KEYMAP.right.key_binding);
  Serial.print("UP:");
  Serial.println(KEYMAP.up.key_binding);
  Serial.print("DOWN:");
  Serial.println(KEYMAP.down.key_binding);
  Serial.println();
  
  Serial.println("Buttons:");
  Serial.println();
  Serial.print("TOP LEFT:");
  Serial.println(KEYMAP.btn_topLeft.key_binding);
  Serial.print("TOP RIGHT:");
  Serial.println(KEYMAP.btn_topRight.key_binding);
  Serial.print("BOTTOM LEFT:");
  Serial.println(KEYMAP.btn_bottomLeft.key_binding);
  Serial.print("BOTTOM RIGHT:");
  Serial.println(KEYMAP.btn_bottomRight.key_binding);
  
  Serial.println();
  Serial.println("COMBOS: (press 2 buttons simultaneously)");
  Serial.println();
  Serial.print("TOP BUTTONS:");
  Serial.println(KEYMAP.C_TOP.key_binding);
  Serial.print("BOTTOM BUTTONS:");
  Serial.println(KEYMAP.C_BOTTOM.key_binding);
  Serial.print("LEFT BUTTONS:");
  Serial.println(KEYMAP.C_LEFT.key_binding);
  Serial.print("RIGHT BUTTONS:");
  Serial.println(KEYMAP.C_RIGHT.key_binding);


}

void loop() //no interrupts, no args, no returns, no bullshit... just a bunch of polling loops.
{

  get_current_state();

  scan_stick();

  scan_buttons();

  scan_combos();    

if (KEYMAP.C_MASH.debounced) prog_mode(); //mash all four buttons for MASH_TIMEOUT milliseconds to enter programming mode
  
}

void get_current_state(void)
{
  if(digitalRead(KEYMAP.left.Leonardo_pin)==LOW) KEYMAP.left.active=true; else KEYMAP.left.active=false;
  if(digitalRead(KEYMAP.right.Leonardo_pin)==LOW) KEYMAP.right.active=true; else KEYMAP.right.active=false;
  if(digitalRead(KEYMAP.up.Leonardo_pin)==LOW) KEYMAP.up.active=true; else KEYMAP.up.active=false;
  if(digitalRead(KEYMAP.down.Leonardo_pin)==LOW) KEYMAP.down.active=true; else KEYMAP.down.active=false;
  if(digitalRead(KEYMAP.btn_topLeft.Leonardo_pin)==LOW) KEYMAP.btn_topLeft.active=true; else KEYMAP.btn_topLeft.active=false;
  if(digitalRead(KEYMAP.btn_topRight.Leonardo_pin)==LOW) KEYMAP.btn_topRight.active=true; else KEYMAP.btn_topRight.active=false;
  if(digitalRead(KEYMAP.btn_bottomLeft.Leonardo_pin)==LOW) KEYMAP.btn_bottomLeft.active=true; else KEYMAP.btn_bottomLeft.active=false;
  if(digitalRead(KEYMAP.btn_bottomRight.Leonardo_pin)==LOW) KEYMAP.btn_bottomRight.active=true; else KEYMAP.btn_bottomRight.active=false;
}

void scan_stick(void)
{
  //if joystick is pushed left
  if(KEYMAP.left.active) Keyboard.press(KEYMAP.left.key_binding);
    else(Keyboard.release(KEYMAP.left.key_binding));
  
  //if joystick is pushed right
 if(KEYMAP.right.active) Keyboard.press(KEYMAP.right.key_binding);
    else(Keyboard.release(KEYMAP.right.key_binding));

  //if joystick is pushed up  
  if(KEYMAP.up.active) Keyboard.press(KEYMAP.up.key_binding);
    else(Keyboard.release(KEYMAP.up.key_binding));

  //if joystick is pushed down
  if(KEYMAP.down.active) Keyboard.press(KEYMAP.down.key_binding);
    else(Keyboard.release(KEYMAP.down.key_binding));
}

void scan_buttons(void)
{
  //if top left button is pressed  
  if(KEYMAP.btn_topLeft.active) Keyboard.press(KEYMAP.btn_topLeft.key_binding);
    else Keyboard.release(KEYMAP.btn_topLeft.key_binding);

  //if top right button is pressed  
  if(KEYMAP.btn_topRight.active) Keyboard.press(KEYMAP.btn_topRight.key_binding);
    else Keyboard.release(KEYMAP.btn_topRight.key_binding);
  
  //if bottom left button is pressed  
  if(KEYMAP.btn_bottomLeft.active) Keyboard.press(KEYMAP.btn_bottomLeft.key_binding);
    else Keyboard.release(KEYMAP.btn_bottomLeft.key_binding);

  //if bottom right button is pressed  
  if(KEYMAP.btn_bottomRight.active) Keyboard.press(KEYMAP.btn_bottomRight.key_binding);
    else Keyboard.release(KEYMAP.btn_bottomRight.key_binding);
}

void scan_combos(void)
{
//checks:

  //TOP:
if(KEYMAP.btn_topLeft.active && KEYMAP.btn_topRight.active) KEYMAP.C_TOP.active=true; 
  else KEYMAP.C_TOP.active=false;

  //BOTTOM:
if(KEYMAP.btn_bottomRight.active && KEYMAP.btn_bottomLeft.active) KEYMAP.C_BOTTOM.active=true; 
  else KEYMAP.C_BOTTOM.active=false;

  //LEFT:
if(KEYMAP.btn_bottomLeft.active && KEYMAP.btn_topLeft.active) KEYMAP.C_LEFT.active=true; 
  else KEYMAP.C_LEFT.active=false;

  //RIGHT:
if(KEYMAP.btn_topRight.active && KEYMAP.btn_bottomRight.active) KEYMAP.C_RIGHT.active=true; 
  else KEYMAP.C_RIGHT.active=false;

  //MASH:
if(KEYMAP.C_TOP.active && KEYMAP.C_BOTTOM.active) //if all 4 buttons are pressed.
//if(KEYMAP.C_LEFT.active && KEYMAP.C_RIGHT.active)  //could have been used just as well... just here to clarify..
{
  KEYMAP.C_MASH.active=true;  //MARK THE MASH COMBO AS ACTIVE, KILL THE REST.
  KEYMAP.C_TOP.active=false;
  KEYMAP.C_BOTTOM.active=false;
  KEYMAP.C_LEFT.active=false;
  KEYMAP.C_RIGHT.active=false;
}
  else KEYMAP.C_MASH.active=false;



//handlers:
    //TOP:
if(KEYMAP.C_TOP.active) 
{
  if(KEYMAP.C_TOP.first) //if this is the first time the combo has been seen
  {
    KEYMAP.C_TOP.first = false;             //next time isn't the first
    KEYMAP.C_TOP.filter_counter = millis(); //start the clock   
  }

    else if((millis() > KEYMAP.C_TOP.filter_counter + COMBO_FILTER_VALUE))
      {
        Keyboard.press(KEYMAP.C_TOP.key_binding);
        return; //ONLY ONE COMBO CAN BE ACTIVE AT ANY GIVEN TIME, think about it..
      }
}
  else 
  {
    Keyboard.release(KEYMAP.C_TOP.key_binding);
    KEYMAP.C_TOP.first=true;
  }

    //BOTTOM:
if(KEYMAP.C_BOTTOM.active) 
{
  if(KEYMAP.C_BOTTOM.first) //if this is the first time the combo has been seen
  {
    KEYMAP.C_BOTTOM.first = false;             //next time isn't the first
    KEYMAP.C_BOTTOM.filter_counter = millis(); //start the clock   
  }

    else if((millis() > KEYMAP.C_BOTTOM.filter_counter + COMBO_FILTER_VALUE))  
      {
        Keyboard.press(KEYMAP.C_BOTTOM.key_binding);
        return; //ONLY ONE COMBO CAN BE ACTIVE AT ANY GIVEN TIME, think about it..
      }
}
  else 
  {
    Keyboard.release(KEYMAP.C_BOTTOM.key_binding);
    KEYMAP.C_BOTTOM.first=true;
  }  


    //LEFT:
if(KEYMAP.C_LEFT.active) 
{
  if(KEYMAP.C_LEFT.first) //if this is the first time the combo has been seen
  {
    KEYMAP.C_LEFT.first = false;             //next time isn't the first
    KEYMAP.C_LEFT.filter_counter = millis(); //start the clock   
  }

    else if((millis() > KEYMAP.C_LEFT.filter_counter + COMBO_FILTER_VALUE)) 
      {
        Keyboard.press(KEYMAP.C_LEFT.key_binding);
        return; //ONLY ONE COMBO CAN BE ACTIVE AT ANY GIVEN TIME, think about it..
      }      
}
  else 
  {
    Keyboard.release(KEYMAP.C_LEFT.key_binding);
    KEYMAP.C_LEFT.first=true;
  }  


    //RIGHT
if(KEYMAP.C_RIGHT.active) 
{
  if(KEYMAP.C_RIGHT.first) //if this is the first time the combo has been seen
  {
    KEYMAP.C_RIGHT.first = false;             //next time isn't the first
    KEYMAP.C_RIGHT.filter_counter = millis(); //start the clock   
  }

    else if((millis() > KEYMAP.C_RIGHT.filter_counter + COMBO_FILTER_VALUE)) 
      {
        Keyboard.press(KEYMAP.C_RIGHT.key_binding);
        return; //ONLY ONE COMBO CAN BE ACTIVE AT ANY GIVEN TIME, think about it..
      }      
}
  else 
  {
    Keyboard.release(KEYMAP.C_RIGHT.key_binding);
    KEYMAP.C_RIGHT.first=true;
  }

    //MASH
if(KEYMAP.C_MASH.active) 
{
  if(KEYMAP.C_MASH.first) //if this is the first time the combo has been seen
  {
    KEYMAP.C_MASH.first = false;             //next time isn't the first
    KEYMAP.C_MASH.filter_counter = millis(); //start the clock   
  }

    else if((millis() > KEYMAP.C_MASH.filter_counter + MASH_TIMEOUT)) 
      {
      KEYMAP.C_MASH.debounced=true;  
      return;
      }
    
}
  else 
  {
    KEYMAP.C_MASH.debounced=false;
    KEYMAP.C_MASH.first=true;
  }

}


void prog_mode(void)
{

Keyboard.releaseAll(); //safety john

int incomingByte = 0;   // for incoming serial data


Serial.println();
Serial.println("NOW ENTERING PROGRAMMABLE MODE, here be dragons.");
Serial.println();

while(true)  //how do i get back out?! think!
        {

        // send data only when you receive data:
        if (Serial.available() > 0) {
                // read the incoming byte:
                incomingByte = Serial.read();

                // say what you got:
                Serial.print("I received: ");
                Serial.println(incomingByte, DEC);
        }
}


}