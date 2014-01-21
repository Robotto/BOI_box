/*
  PROGRAMMABLE JOYSTICK.

Makes a joystick with 1 (digital)stick and 4 buttons into a programmable controller
default values are for the for the game 'Binding of Isaac'.

NOTE: This code will work on Arduino Leonardo and compatible boards, because they are able to act as native 
keyboard and/or mouse devices.

Created 2012
by mark@moore.dk

Feel free to use this code and edit, sell, buy, republish-taking-credit or print-out-and-eat it.
*/

//for the programmer:
//priority (order):
//[LEFT] [RIGHT] [UP] [DOWN] [TOP LEFT BUTTON] [TOP RIGHT BUTTON] [BOTTOM LEFT BUTTON] [BOTTOM RIGHT BUTTON] [LEFT COMBO] [RIGHT COMBO] [TOP COMBO] [BOTTOM COMBO]

#include <EEPROM.h> //remembers stuff without power, slow reads, slower writes, limited number of writes ~10^4


#define NUM_STICK_POSITIONS 4    //the number of stick positions (4)
#define NUM_BUTTONS 4            //the number of buttons (4 buttons)
#define NUM_COMBOS  4            //the number of button combinations (TOP,BOTTOM,LEFT,RIGHT)
#define TEKKEN_FILTER_VALUE 10L  //miliseconds to wait for button
#define DEBOUNCE_MS     10L



//let's keep all the joystick info nice and tidy in some struct magic:
//IF YOU UNDERSTAND THE STRUCT, YOU UNDERSTAND THE CODE, IF NOT; NOT.


typedef struct //stick positions
{
  int EEPROM_ADDR;
  char key_binding;
  int Leonardo_pin;
  bool active;
  bool keypress_sent;      //true if a keypress has been sent, false if a key-release has been sent
  bool last_state;
  long millis_at_last_change;
} STICK;


typedef struct //combos
{
  int EEPROM_ADDR;
  char key_binding;
  bool active;
  bool keypress_sent;      //true if a keypress has been sent, false if a key-release has been sent
} COMBO;

typedef struct //regular buttons 
  {
  int EEPROM_ADDR; //associated EEPROM address - where the keybinding is stored
  char key_binding; //actual key-binding, has a default value set in code, but loads from eeprom if available
  int Leonardo_pin; //pin number on the lEO board
  bool active; //is this button currently activated?
  bool keypress_sent; //true if a keypress has been sent, false if a key-release has been sent
  bool first;           //these booleans are used as sort of a debouncer, to check if a button is pressed that isn't part of a combo, or just a result of someone's girlfriend playing tekken...
                        //combined with these 'counters' we get an actual debounce. henceforth known as the tekken-filter.
  long filter_counter;  //variable to store the value of the long millis(); when an input is activated
  bool last_state;
  long millis_at_last_change;
  } BUTTON; 


  STICK left;             //joystick left
  STICK right;            //joystick right
  STICK up;               //joystick up
  STICK down;             //joystick down

  COMBO C_LEFT;           //lefty losey
  COMBO C_RIGHT;          //righty tighty
  COMBO C_TOP;            //top
  COMBO C_BOTTOM;         //bottom. p00t!

  BUTTON btn_topLeft;     //top left button
  BUTTON btn_topRight;    //top right button
  BUTTON btn_bottomLeft;  //bottom left button
  BUTTON btn_bottomRight; //bottom right button

  

//Make some arrays of pointers that point to the stuff above. this makes it easy to do bulk operations (with for loops),
//but also allows for manipulation of a single adressable item... the best of both worlds :)
//A quick explanation of struct pointers: http://www.eskimo.com/~scs/cclass/int/sx1d.html

STICK *stick_map[NUM_STICK_POSITIONS]={&left,&right,&up,&down};                         //stick_map
COMBO *combo_map[NUM_COMBOS]={&C_LEFT,&C_RIGHT,&C_TOP,&C_BOTTOM};                      //combo_map
BUTTON *button_map[NUM_BUTTONS] ={&btn_topLeft,&btn_topRight,&btn_bottomLeft,&btn_bottomRight}; //button_map

//REMAP of keys that can't be sent by terminal (up, down left, right, 2xctrl, 2xalt, 2xshift:
//This is done so that the joystick can be reprogrammed using the terminal. (serial port)

const char left_remap =  (char)17; //ASCII 17(dec) = Device control 1
const char right_remap = (char)18; //ASCII 18(dec) = Device control 2
const char up_remap =    (char)19; //ASCII 19(dec) = Device control 3
const char down_remap =  (char)20; //ASCII 20(dec) = Device control 4

const char left_shift_remap =  (char)14; //ASCII = Shift out
const char right_shift_remap = (char)15; //ASCII = Shift in
const char left_ctrl_remap =   (char)16; //ASCII = Data link escape
const char right_ctrl_remap =  (char)21; //ASCII = Negative acknowledge
const char left_alt_remap =    (char)22; //ASCII = Sunchronous idle
const char right_alt_remap =   (char)23; //ASCII = End of transmission block


void setup() {

/*
 //test dummy write to flash. .. dont overuse this... there's a limited number of flash writes available before it wears out.
  for (int i = 0; i < 256; i++)
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

  left.EEPROM_ADDR=0;
  right.EEPROM_ADDR=1;
  up.EEPROM_ADDR=2;
  down.EEPROM_ADDR=3;

  btn_topLeft.EEPROM_ADDR=4;
  btn_topRight.EEPROM_ADDR=5;
  btn_bottomLeft.EEPROM_ADDR=6;
  btn_bottomRight.EEPROM_ADDR=7;

  C_TOP.EEPROM_ADDR=8;
  C_BOTTOM.EEPROM_ADDR=9;
  C_LEFT.EEPROM_ADDR=10;
  C_RIGHT.EEPROM_ADDR=11;


//RESET DEBOUNCE COUNTERS:
  for (int i = 0; i < NUM_STICK_POSITIONS; i++)
  {
    stick_map[i]->millis_at_last_change=0;
  }
  for (int i = 0; i < NUM_BUTTONS; i++) 
  {
    button_map[i]->millis_at_last_change=0;
  }
  


  //NOW FOR THE MAGIC:
  //load values from EEPROM... verify that they're not 0xFF (unwritten memory) 
  //and apply either loaded value or default value depending on verification.

char temp_key_placeholder; //to avoid reading more than once per binding from the PROM

//TODO: a more extensive check than just !=0xff .. other values will kill.. check for them.

  for (int i = 0; i < NUM_STICK_POSITIONS; i++) //key bindings for the stick:
  {
  temp_key_placeholder=(char)EEPROM.read(stick_map[i]->EEPROM_ADDR);    
  if (temp_key_placeholder!=0xFF) stick_map[i]->key_binding=temp_key_placeholder;
  }
  
  for (int i = 0; i < NUM_BUTTONS; i++) //key bindings for the buttons
  {
  temp_key_placeholder=(char)EEPROM.read(button_map[i]->EEPROM_ADDR);    
  if (temp_key_placeholder!=0xFF) button_map[i]->key_binding=temp_key_placeholder;
  }

  for (int i = 0; i < NUM_COMBOS; i++) //key bindings for the combos
  {
  temp_key_placeholder=(char)EEPROM.read(combo_map[i]->EEPROM_ADDR);
  if (temp_key_placeholder!=0xFF) combo_map[i]->key_binding=temp_key_placeholder;
  }

  //woosh...  


  // Set pins to input, and enable the internal pullup resistors, so the input is high unless connected to ground:
  for (int i = 0; i < NUM_STICK_POSITIONS; i++)
  {
    pinMode(stick_map[i]->Leonardo_pin,INPUT_PULLUP);
  }

  for (int i = 0; i < NUM_BUTTONS; i++)
  {
    pinMode(button_map[i]->Leonardo_pin,INPUT_PULLUP);
  }
  

  
  Keyboard.begin(); //for keyboard presses

  Serial.begin(9600); //for reprogramming.

  //delay(5000); //well.. that's just the way it is..

  print_wake_up_msg();


}

char incoming=0; //for serial data.
bool bool_prog_mode=false; //is set true when entering prog_mode(), and false on exit.
bool global_combo_flag=false; //is set if a combo is active, to prevent single button presses to be registered.

void loop() //no interrupts, no args, no returns, no bullshit... just a bunch of polling loops.
{

  if (Serial.available()>0) 
  {
   incoming = Serial.read();
   if ((incoming=='?') && (bool_prog_mode==false)) set_prog_mode(true); //if not in prog mode, expect the first recieved char to be '?'
   if (incoming==4) set_prog_mode(false); //4 = ASCII EOT.. (END OF TRANSMISSION)
  }

  scan_inputs();

  handle_stick(); //stick moves are instant

  check_for_combos(); //combos take priority over single button presses

  handle_combos();

  handle_buttons(); //if no combos, handle buttons, also delay them a bit. (TEKKEN_FILTER_VALUE)

}


void scan_inputs(void)
{
  for (int i = 0; i < NUM_STICK_POSITIONS; i++)
  { 
   if(millis()>(stick_map[i]->millis_at_last_change+DEBOUNCE_MS)) 
    {

      if((digitalRead(stick_map[i]->Leonardo_pin)==LOW) && (stick_map[i]->active==false)) 
        {
        stick_map[i]->active=true;
        stick_map[i]->millis_at_last_change=millis();
        }
      else if(digitalRead(stick_map[i]->Leonardo_pin)==HIGH) 
        {
        stick_map[i]->active=false;
        stick_map[i]->millis_at_last_change=millis();   
        }
    }
  }

  for (int i = 0; i < NUM_BUTTONS; i++)
  { 
   if(millis()>(button_map[i]->millis_at_last_change+DEBOUNCE_MS)) 
    {
      if((digitalRead(button_map[i]->Leonardo_pin)==LOW) && (button_map[i]->active==false)) 
        {
        button_map[i]->active=true;
        button_map[i]->millis_at_last_change=millis();
        }
      else if(digitalRead(button_map[i]->Leonardo_pin)==HIGH) 
        {
        button_map[i]->active=false;
        button_map[i]->millis_at_last_change=millis();   
        }
    }
  }

}

void handle_stick(void)
{

for (int i = 0; i < NUM_STICK_POSITIONS; i++)
  {
    if(stick_map[i]->active && !(stick_map[i]->keypress_sent)) //is it active and has a keypress NOT been sent already
      {
        if (bool_prog_mode==true) //PROGRAMMING MODE
          {
            Serial.print(stick_map[i]->key_binding);
            while(Serial.available()==0);                 //DANGERZONE.
            stick_map[i]->key_binding=(char)Serial.read();
          }
        else //NORMAL OPERATION
          {
            Keyboard.press(remapchecker(stick_map[i]->key_binding));
          }

        stick_map[i]->keypress_sent=true; //remember that a keypress was sent
      }
      else if (!(stick_map[i]->active) && (stick_map[i]->keypress_sent)) //if a key is no longer activated, but a keypress was sent before (because it was active during last check)
        {
          if(bool_prog_mode==false)
           {
            Keyboard.release(remapchecker(stick_map[i]->key_binding));
           }
          stick_map[i]->keypress_sent=false; //remember that a key-release was sent
          //delay(25); //quick and dirty fix.. i'm sorry.
        }
  }
}

void check_for_combos(void)
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

//if any combos are active, set the global combo flag - this prevents the button presses from activating.
if(C_TOP.active||C_BOTTOM.active||C_LEFT.active||C_RIGHT.active) global_combo_flag=true;
else global_combo_flag=false;


}

void handle_combos(void)
{
//handler:
    
  for (int i = 0; i < NUM_COMBOS; i++)
  {
    if(combo_map[i]->active && !(combo_map[i]->keypress_sent)) //is it active and has a keypress NOT been sent already?
      {
        if (bool_prog_mode==true) 
          {
            Serial.print(combo_map[i]->key_binding);
            while(Serial.available()==0);                 //DANGERZONE.
            combo_map[i]->key_binding=(char)Serial.read();
          }
        else 
          {
            Keyboard.press(remapchecker(combo_map[i]->key_binding));
          }

        combo_map[i]->keypress_sent=true; //remember that a keypress was sent
      }
      else if (!(combo_map[i]->active) && (combo_map[i]->keypress_sent)) //if a key is no longer activated, but a keypress was sent before (because it was acitve during last check)?
        {
          if(bool_prog_mode==false)
           {
            Keyboard.release(remapchecker(combo_map[i]->key_binding));
           }
          combo_map[i]->keypress_sent=false; //remember that a key-release was sent
        }
  }
}

void handle_buttons(void) //if a button remains depressed for the duration of the TEKKEN_FILTER_VALUE miliseconds (default value is 50mS), send a keypress.
{

for (int i = 0; i < NUM_BUTTONS; i++)
  {
    if(button_map[i]->active && !(global_combo_flag)) //if the button is active AND no combos are active.
    {
      if(button_map[i]->first) //if this is the first time the buttonpress is seen
      {
        button_map[i]->first = false;             //next time isn't the first
        button_map[i]->filter_counter = millis(); //start the clock   
      }

      else if((millis() > button_map[i]->filter_counter + TEKKEN_FILTER_VALUE) && !(button_map[i]->keypress_sent))
      {
        if (bool_prog_mode==true) 
          {
            Serial.print(button_map[i]->key_binding);
            while(Serial.available()==0);                 //DANGERZONE.
            button_map[i]->key_binding=Serial.read();
          }
        else 
          {
            Keyboard.press(remapchecker(button_map[i]->key_binding));
          }

        button_map[i]->keypress_sent=true; //rememer that a keypress was sent
      }
    }
  
    else if(button_map[i]->keypress_sent)
      {
      if(bool_prog_mode==false) 
        {
            Keyboard.release(remapchecker(button_map[i]->key_binding));
        }
      button_map[i]->first=true;
      button_map[i]->keypress_sent=false;
      }
    else button_map[i]->first=true;
  }
}




//this function checks for a remapped key (the keys that can't be sent via terminal) and returns the intended key to be pressed

//to summarize:
/*
//REMAP of keys that can't be sent by terminal (up, down left, right, 2xctrl, 2xalt, 2xshift:
char left_remap =  (char)17; //ASCII 17(dec) = Device control 1
char right_remap = (char)18; //ASCII 18(dec) = Device control 2
char up_remap =    (char)19; //ASCII 19(dec) = Device control 3
char down_remap =  (char)20; //ASCII 20(dec) = Device control 4

char left_shift_remap =  (char)14; //ASCII = Shift out
char right_shift_remap = (char)15; //ASCII = Shift in
char left_ctrl_remap =   (char)16; //ASCII = Data link escape
char right_ctrl_remap =  (char)21; //ASCII = Negative acknowledge
char left_alt_remap =    (char)22; //ASCII = Sunchronous idle
char right_alt_remap =   (char)23; //ASCII = End of transmission block*/

char remapchecker(char key_bind)
{
  char key_to_send;
  switch(key_bind)
  {
    case left_remap:
    key_to_send=KEY_LEFT_ARROW;
    break;

    case right_remap:
    key_to_send=KEY_RIGHT_ARROW;
    break;

    case up_remap:
    key_to_send=KEY_UP_ARROW;
    break;

    case down_remap:
    key_to_send=KEY_DOWN_ARROW;
    break;

    case left_shift_remap:
    key_to_send=KEY_LEFT_SHIFT;
    break;

    case right_shift_remap:
    key_to_send=KEY_RIGHT_SHIFT;
    break;

    case left_ctrl_remap:
    key_to_send=KEY_LEFT_CTRL;
    break;

    case right_ctrl_remap:
    key_to_send=KEY_RIGHT_CTRL;
    break;

    case left_alt_remap:
    key_to_send=KEY_LEFT_ALT;
    break;

    case right_alt_remap:
    key_to_send=KEY_RIGHT_ALT;
    break;

    default:
    key_to_send=key_bind;
    break;
  }  
  return key_to_send;
}




void set_prog_mode(bool enabled)
{

if(enabled==false)
  {

    bool_prog_mode=false;

    for (int i = 0; i < NUM_STICK_POSITIONS; i++) //key bindings for the stick:
    {
    EEPROM.write(stick_map[i]->EEPROM_ADDR,(byte)stick_map[i]->key_binding);    //cast keybind to byte, write eeprom.
    }
  
    for (int i = 0; i < NUM_BUTTONS; i++) //key bindings for the buttons
    {
    EEPROM.write(button_map[i]->EEPROM_ADDR,(byte)button_map[i]->key_binding);    //cast keybind to byte, write eeprom.
    }

    for (int i = 0; i < NUM_COMBOS; i++) //key bindings for the combos
    {
    EEPROM.write(combo_map[i]->EEPROM_ADDR,(byte)combo_map[i]->key_binding);    //cast keybind to byte, write eeprom.
    }

    Serial.print((char)6); //ACK
  }

else
  {

bool_prog_mode=true;

Keyboard.releaseAll(); //safety john

//RESPOND TO PC by telling it what the current config is:
//ORDER: [LEFT] [RIGHT] [UP] [DOWN] [TOP LEFT BUTTON] [TOP RIGHT BUTTON] [BOTTOM LEFT BUTTON] [BOTTOM RIGHT BUTTON] [LEFT COMBO] [RIGHT COMBO] [TOP COMBO] [BOTTOM COMBO]
  for (int i = 0; i < NUM_STICK_POSITIONS; i++) //key bindings for the stick:
  {
  Serial.print(stick_map[i]->key_binding);
  }
  
  for (int i = 0; i < NUM_BUTTONS; i++) //key bindings for the buttons
  {
  Serial.print(button_map[i]->key_binding);
  }

  for (int i = 0; i < NUM_COMBOS; i++) //key bindings for all the combos
  {
  Serial.print(combo_map[i]->key_binding);
  }

  Serial.println(); //end of line.
  }
}


void print_wake_up_msg()
{
  Serial.println("Goodmorning Human. Shall we play a game?");
  Serial.println("My current configuration is as follows:");
  
  Serial.println("Stick:");
  Serial.println();
  Serial.print("LEFT:");
  Serial.println(left.key_binding);
  Serial.print("RIGHT:");
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