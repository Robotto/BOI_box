/*
  Binding of Isaac controller

Makes a joystick with 1 stick and 4 buttons a controller for the game 'Binding of Isaac'.

NOTE: This code will work on Arduino Leonardo and compatible boards, because they are able to act as native 
keyboard and/or mouse devices.

Created 2012
by Esben Hardenberg and Mark Moore

http://blog.mememaker.com/2012/binding-of-isaac-joystick/
*/

boolean topFirst = true; //flag to test if the top buttons go from unpressed to pressed state
boolean bottomFirst = true; //flag to test if the bottom buttons go from unpressed to pressed state
boolean rightFirst = true; //flag to test if the right buttons go from unpressed to pressed state
long topTimer; //variable to store the value of the long millis(); when the two top buttons are pressed
long bottomTimer; //variable to store the value of the long millis(); when the two bottom buttons are pressed
long rightTimer; //variable to store the value of the long millis(); when the two right buttons are pressed

void setup() {
  // turn on the pullup resistors so it goes high unless connected to ground:
  pinMode(0, INPUT_PULLUP); //Joystick left
  pinMode(1, INPUT_PULLUP); //Joystick right
  pinMode(2, INPUT_PULLUP); //Joystick up
  pinMode(3, INPUT_PULLUP); //Joystick down
  pinMode(4, INPUT_PULLUP); //Top left button
  pinMode(5, INPUT_PULLUP); //Top right button
  pinMode(6, INPUT_PULLUP); //Bottom left button
  pinMode(7, INPUT_PULLUP); //Bottom right button

  //Keyboard presses
  Keyboard.begin();
}

void loop() {
  //if joystick is pushed left
  if(digitalRead(0)==LOW){
    Keyboard.press('a');
  }
  else(Keyboard.release('a'));
  
  //if joystick is pushed right
  if(digitalRead(1)==LOW){
    Keyboard.press('d');
  }
  else(Keyboard.release('d'));

  //if joystick is pushed up  
  if(digitalRead(2)==LOW){
    Keyboard.press('w');
  }
  else(Keyboard.release('w'));

  //if joystick is pushed down
  if(digitalRead(3)==LOW){
    Keyboard.press('s');
  }
  else(Keyboard.release('s'));

  //if top left button is pressed  
  if(digitalRead(4)==LOW){
    Keyboard.press(KEY_LEFT_ARROW);
  }
  else(Keyboard.release(KEY_LEFT_ARROW));

  //if top right button is pressed  
  if(digitalRead(5)==LOW){
    Keyboard.press(KEY_UP_ARROW);
  }
  else(Keyboard.release(KEY_UP_ARROW));
  
  //if bottom left button is pressed  
  if(digitalRead(6)==LOW){
    Keyboard.press(KEY_DOWN_ARROW);
  }
  else(Keyboard.release(KEY_DOWN_ARROW));

  //if bottom right button is pressed  
  if(digitalRead(7)==LOW){
    Keyboard.press(KEY_RIGHT_ARROW);
  }
  else(Keyboard.release(KEY_RIGHT_ARROW));

  //if top left and top right button is pressed: save the uptime of the program in milliseconds 
  //in the variable topTimer and set topFirst to false, so as to not do this again in the next loop  
  if(digitalRead(5)==LOW && digitalRead(4)==LOW && topFirst == true) {
    topFirst = false;
    topTimer = millis();
  }
  
  //If more than 100 milliseconds has elapsed since both top buttons were pressed, press (and hold) the 'e' key. 
  if(digitalRead(5)==LOW && digitalRead(4)==LOW) {
    if(millis() > topTimer + 100) {
      Keyboard.press('e');
    }
  }
  else topFirst=true;
  
  //If one of the top buttons are released, release 'e' key
  if (digitalRead(5)==HIGH | digitalRead(4)==HIGH) {
      Keyboard.release('e');
  }
    
  //if bottom left and bottom right button is pressed: save the uptime of the program in milliseconds 
  //in the variable topTimer and set topFirst to false, so as to not do this again in the next loop  
  if(digitalRead(6)==LOW && digitalRead(7)==LOW && bottomFirst == true) {
    bottomFirst = false;
    bottomTimer = millis();
  }
  
  //If more than 100 milliseconds has elapsed since both bottom buttons were pressed, press (and hold) the Space key (ASCII code 32). 
  if(digitalRead(6)==LOW && digitalRead(7)==LOW) {
    if(millis() > bottomTimer + 100) {
      Keyboard.press(32);
    }
  }
  else bottomFirst=true;
  
  //If one of the bottom buttons are released, release Space key
  if (digitalRead(6)==HIGH | digitalRead(7)==HIGH) {
      Keyboard.release(32);
  }

  //if bottom right and top right button is pressed: save the uptime of the program in milliseconds 
  //in the variable topTimer and set topFirst to false, so as to not do this again in the next loop  
  if(digitalRead(5)==LOW && digitalRead(7)==LOW && rightFirst == true) {
    rightFirst = false;
    rightTimer = millis();
  }
  
  //If more than 100 milliseconds has elapsed since both bottom buttons were pressed, press (and hold) the 'q' key (ASCII code 32). 
  if(digitalRead(5)==LOW && digitalRead(7)==LOW) {
    if(millis() > rightTimer + 100) {
      Keyboard.press('q');
    }
  }
  else rightFirst=true;
  
  //If one of the bottom buttons are released, release 'q' key
  if (digitalRead(5)==HIGH | digitalRead(7)==HIGH) {
      Keyboard.release('q');
  }
}
