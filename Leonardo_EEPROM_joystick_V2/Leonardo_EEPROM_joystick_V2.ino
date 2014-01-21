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

#define NUM_INPUTS  8            //the number of inputs we have (4 buttons and 4 joystick positions)
#define NUM_COMBOS  5            //the number of button combinations (TOP,BOTTOM,LEFT,RIGHT,MASH)
#define COMBO_FILTER_VALUE 100L  //miliseconds to wait for combo
#define MASH_TIMEOUT 3000L       //mash all four buttons for MASH_TIMEOUT milliseconds to enter programming mode:

//let's keep all the joystick info nice and tidy in some struct magic:
//IF YOU UNDERSTAND THE STRUCT, YOU UNDERSTAND THE CODE, IF NOT; NOT.

typedef struct //regular keys  
  {
  int EEPROM_ADDR; //associated EEPROM address - where the keybinding is stored
  char key_binding; //actual key-binding, has a default value set in code, but loads from eeprom if available
  int Leonardo_pin; //pin number on the lEO board
  bool active; //is this button currently activated?
  bool last_state; //true if a keypress has been sent, false if a key-release has been sent
  } BINDING; 

typedef struct //combos
{
  int EEPROM_ADDR;
  char key_binding;
  bool active;
//  bool debounced;       //only used with the MASH COMBO.. it's handled a bit differently because it doesn't send a keypress.
  bool first;           //these booleans are used as sort of a debouncer, to check if the combo was on purpose, or just a result of someone's girlfriend playing tekken...
                        //combined with these 'counters' we get an actual debounce. henceforth known as the tekken-filter.
  long filter_counter;  //variable to store the value of the long millis(); when the two combo buttons are pressed
  bool last_state;      //true if a keypress has been sent, false if a key-release has been sent
} COMBO;


  BINDING left;            //joystick left
  BINDING right;           //joystick right
  BINDING up;              //joystick up
  BINDING down;            //joystick down
  BINDING btn_topLeft;     //top left button
  BINDING btn_topRight;    //top right button
  BINDING btn_bottomLeft;  //bottom left button
  BINDING btn_bottomRight; //bottom right button

  COMBO    C_TOP;           //yup.. press the two buttons at the top for a brand new key.
  COMBO    C_BOTTOM;        //same goes for the bottom. p00t!
  COMBO    C_LEFT;          //lefty losey
  COMBO    C_RIGHT;         //righty tighty

  COMBO    C_MASH;          //ALL FOUR BUTTONS

//MAke some arrays of pointers that point to the stuff above. this makes it easy to do bulk operations (with for loops),
//but also allows for manipulation of a single adressable item... the best of both worlds :)
//A quick explanation of struct pointers: http://www.eskimo.com/~scs/cclass/int/sx1d.html

BINDING *joystick_map[NUM_INPUTS] ={&left,&right,&up,&down,&btn_topLeft,&btn_topRight,&btn_bottomLeft,&btn_bottomRight};
COMBO *combo_map[NUM_COMBOS]={&C_TOP,&C_BOTTOM,&C_LEFT,&C_RIGHT,&C_MASH};


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

  left.key_binding='a';
  right.key_binding='d';
  up.key_binding='w';
  down.key_binding='s';
  btn_topLeft.key_binding=KEY_LEFT_ARROW;
  btn_topRight.key_binding=KEY_UP_ARROW;
  btn_bottomLeft.key_binding=KEY_DOWN_ARROW;
  btn_bottomRight.key_binding=KEY_RIGHT_ARROW;
  C_TOP.key_binding='e';
  C_BOTTOM.key_binding=32; //Where do astronauts get drunk? ....spacebar. :D
  C_LEFT.key_binding='r';
  C_RIGHT.key_binding='q';
  //END OF DEFAULTS.

  //ARDUINO LEONARDO BUTTON MAP - aka. where did you solder the wires to?
  left.Leonardo_pin=0;
  right.Leonardo_pin=1;
  up.Leonardo_pin=2;
  down.Leonardo_pin=3;
  btn_topLeft.Leonardo_pin=4;
  btn_topRight.Leonardo_pin=5;
  btn_bottomLeft.Leonardo_pin=6;
  btn_bottomRight.Leonardo_pin=7;
  //END OF LEONARDO PINMAP

  //EEPROM ADDRESSES: where the programmed keypresses are stored
  //TOTALLY TO BE FUCKED WITH. Go ahead.. try.
  for (int i = 0; i < NUM_INPUTS; ++i) //0-7
  {
    joystick_map[i]->EEPROM_ADDR=i;
  }
  for (int i = 0; i < NUM_COMBOS-1; ++i) //8-11 - the mash combo doesn't have a keypress, it's the trigger for program mode.
  {
    combo_map[i]->EEPROM_ADDR=i+NUM_INPUTS;
  }


    //NOW FOR THE MAGIC:
  //load values from EEPROM... verify that they're not 0xFF (unwritten memory) 
  //and apply either loaded value or default value depending on verification.

char temp_key_placeholder; //to avoid reading more than once per binding from the PROM

//TODO: a more extensive check than just !=0xff .. other values will kill.. check for them.

  for (int i = 0; i < NUM_INPUTS; ++i) //key bindings for the buttons and stick:
  {
  temp_key_placeholder=(char)EEPROM.read(joystick_map[i]->EEPROM_ADDR);    
  if (temp_key_placeholder!=0xFF) joystick_map[i]->key_binding=temp_key_placeholder;
  }
  
  for (int i = 0; i < NUM_COMBOS-1; ++i) //key bindings for all the combos except MASH
  {
  temp_key_placeholder=(char)EEPROM.read(combo_map[i]->EEPROM_ADDR);
  if (temp_key_placeholder!=0xFF) combo_map[i]->key_binding=temp_key_placeholder;
  }

  //woosh...  


  // turn on the pullup resistors so it goes high unless connected to ground:
  for (int i = 0; i < NUM_INPUTS; ++i)
  {
    pinMode(joystick_map[i]->Leonardo_pin,INPUT_PULLUP);
  }

  //Keyboard presses
  Keyboard.begin();

  Serial.begin(9600); //will seriously fuck with the arduino IDE.. 

  delay(5000); //...which is why this is here...

  Serial.println("Goodmorning Human. Shall we play a game?");
  Serial.println("My current configuration is as follows:");
  
  Serial.println("Stick:");
  Serial.println();
  Serial.print("LEFT:");
  Serial.println(left.key_binding);
  Serial.print("RIGHT");
  Serial.println(right.key_binding);
  Serial.print("UP:");
  Serial.println(up.key_binding);
  Serial.print("DOWN:");
  Serial.println(down.key_binding);
  Serial.println();
  
  Serial.println("Buttons:");
  Serial.println();
  Serial.print("TOP LEFT:");
  Serial.println(btn_topLeft.key_binding);
  Serial.print("TOP RIGHT:");
  Serial.println(btn_topRight.key_binding);
  Serial.print("BOTTOM LEFT:");
  Serial.println(btn_bottomLeft.key_binding);
  Serial.print("BOTTOM RIGHT:");
  Serial.println(btn_bottomRight.key_binding);
  
  Serial.println();
  Serial.println("COMBOS: (press 2 buttons simultaneously)");
  Serial.println();
  Serial.print("TOP BUTTONS:");
  Serial.println(C_TOP.key_binding);
  Serial.print("BOTTOM BUTTONS:");
  Serial.println(C_BOTTOM.key_binding);
  Serial.print("LEFT BUTTONS:");
  Serial.println(C_LEFT.key_binding);
  Serial.print("RIGHT BUTTONS:");
  Serial.println(C_RIGHT.key_binding);


}

void loop() //no interrupts, no args, no returns, no bullshit... just a bunch of polling loops.
{

  get_current_state(); //reads inputs

  handle_stick_and_buttons(); //decides what to do with the inputs

  scan_combos();  //checks for combos, with a tiny delay, to avoid accidental combos.
}

void get_current_state(void)
{
  for (int i = 0; i < NUM_INPUTS; ++i)
  {
    if(digitalRead(joystick_map[i]->Leonardo_pin)==LOW) joystick_map[i]->active=true; else joystick_map[i]->active=false;
  }
}

void handle_stick_and_buttons(void)
{
  for (int i = 0; i < NUM_INPUTS; ++i)
  {
    if(joystick_map[i]->active && !(joystick_map[i]->last_state)) //is it active and has a keypress NOT been sent already
      {
        Keyboard.press(joystick_map[i]->key_binding);
        joystick_map[i]->last_state=true; //remember that a keypress was sent
      }
      else if (joystick_map[i]->last_state) //has a keypress been sent before?
        {
          Keyboard.release(joystick_map[i]->key_binding);
          joystick_map[i]->last_state=false; //remember that a key-release was sent
        }
  }
}


void scan_combos(void)
{
//checks:
  //TOP:
if(btn_topLeft.active && btn_topRight.active) C_TOP.active=true; 
  else C_TOP.active=false;

  //BOTTOM:
if(btn_bottomRight.active && btn_bottomLeft.active) C_BOTTOM.active=true; 
  else C_BOTTOM.active=false;

  //LEFT:
if(btn_bottomLeft.active && btn_topLeft.active) C_LEFT.active=true; 
  else C_LEFT.active=false;

  //RIGHT:
if(btn_topRight.active && btn_bottomRight.active) C_RIGHT.active=true; 
  else C_RIGHT.active=false;

  //MASH:
if(C_TOP.active && C_BOTTOM.active) //if all 4 buttons are pressed.
//if(C_LEFT.active && C_RIGHT.active)  //could have been used just as well... just here to clarify..
{
  C_MASH.active=true;  //MARK THE MASH COMBO AS ACTIVE, KILL THE REST.
  C_TOP.active=false;
  C_BOTTOM.active=false;
  C_LEFT.active=false;
  C_RIGHT.active=false;
}
  else C_MASH.active=false;



//handler:
    

for (int i = 0; i < NUM_COMBOS-1; ++i) //MASH COMBO is handled differrently
{
if(combo_map[i]->active) 
{
  if(combo_map[i]->first) //if this is the first time the combo has been seen
  {
    combo_map[i]->first = false;             //next time isn't the first
    combo_map[i]->filter_counter = millis(); //start the clock   
  }

    else if((millis() > combo_map[i]->filter_counter + COMBO_FILTER_VALUE) && !(combo_map[i]->last_state))
      {
        Keyboard.press(combo_map[i]->key_binding);
        combo_map[i]->last_state=true; //rememer that a keypress was sent
        return; //ONLY ONE COMBO CAN BE ACTIVE AT ANY GIVEN TIME, think about it..
      }
}
  else if(combo_map[i]->last_state)
  {
    combo_map[i]->first=true;
    Keyboard.release(combo_map[i]->key_binding);
    combo_map[i]->last_state=false;
  }
  else combo_map[i]->first=true;
}

    //MASH
if(C_MASH.active) 
{
  if(C_MASH.first) //if this is the first time the combo has been seen
  {
    C_MASH.first = false;             //next time isn't the first
    C_MASH.filter_counter = millis(); //start the clock   
  }

    else if((millis() > C_MASH.filter_counter + MASH_TIMEOUT)) 
      {
      prog_mode(); //ENTER PROGRAMMING MODE
      return;
      }
    
}
  else 
  {
    C_MASH.first=true;
  }

}


void prog_mode(void)
{

//TODO: better validity check of incoming serial data before assignment to kyebind.

bool stay_in_prog_mode=true;
Keyboard.releaseAll(); //safety john

int incomingByte = 0;   // for incoming serial data
char incoming=0;

Serial.println();
Serial.println("NOW ENTERING PROGRAMMABLE MODE, you're not in Kansas anymore...");
Serial.println();
Serial.println("Expecting a string where each char represents a new keybinding in the order:");
Serial.println("[LEFT] [RIGHT] [UP] [DOWN] [TOP LEFT BUTTON] [TOP RIGHT BUTTON] [BOTTOM LEFT BUTTON] [BOTTOM RIGHT BUTTON] [TOP COMBO] [BOTTOM COMBO] [LEFT COMBO] [RIGHT COMBO]");
Serial.println();
Serial.println("current configuration:");

for (int i = 0; i < NUM_INPUTS; ++i) //print current joystick keybind configuration
{
  Serial.print(joystick_map[i]->key_binding);
}
for (int i = 0; i < NUM_COMBOS-1; ++i) //print current combo keybind configuration (again, last combo doesn't have a bind)
{
  Serial.print(combo_map[i]->key_binding);
}
Serial.println(); //end line.

while(stay_in_prog_mode) 
        {
        if (Serial.available() > 11) //wait for the entire string to come in.
        {

                
                for (int i = 0; i < NUM_INPUTS; ++i) //for the inputs
                {
                  incoming = (char)Serial.read(); //read one incoming byte, cast to char.

                  if ((incoming!=0)&&(incoming!='\n')&&(incoming!='\r')) //check for valdity .. this could be better..
                  {
                    joystick_map[i]->key_binding=Serial.read();                               //Read the incoming bytes into keybinds
                    EEPROM.write(joystick_map[i]->EEPROM_ADDR,joystick_map[i]->key_binding);  //Save the new keybinds to EEPROM.
                  }
                  else Serial.println("ERROR!: bad char or wrong string length!");
                  
                }
                for (int i = 0; i < NUM_COMBOS-1; ++i) //for the combos
                {

                  incoming = (char)Serial.read(); 

                  if ((incoming!=0)&&(incoming!='\n')&&(incoming!='\r')) 
                  {
                  combo_map[i]->key_binding=Serial.read();
                  EEPROM.write(combo_map[i]->EEPROM_ADDR,combo_map[i]->key_binding);
                  }
                  else Serial.println("ERROR!: bad char or wrong string length!");
                }

                stay_in_prog_mode=false; //if we reach here, all should be well.. :/

        }
}


}