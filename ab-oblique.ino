// Carles Bellver Torlà
// carles@carlesbellver.net
// Oblique Strategies for Arduboy
// version 0.2.1
// 2020-01-18

#define FSEC 10 // frames per second
#define EEPROM_START EEPROM_STORAGE_SPACE_START

#include <avr/pgmspace.h>
#include <EEPROM.h>
#include <Arduboy2.h>
Arduboy2 ab;
BeepPin1 beep;
#include <Arduboy2Core.h>
Arduboy2Core core;
#include <Tinyfont.h>
Tinyfont tf = Tinyfont(ab.sBuffer, ab.width(), ab.height());
#include "strategies.h"

#define TIMES_SIZE 18
uint16_t times[TIMES_SIZE] = {0,1,2,3,4,5,10,15,30,60,120,180,240,300,600,900,1200,1800};
uint8_t min_c=8; uint16_t min=30*FSEC;  // 30 seconds - time you must wait to request a new strategy
uint8_t max_c=11; uint16_t max=180*FSEC; //  3 minutes - max time before a new strategy is displayed
uint16_t start=3*FSEC; //  3 seconds - title screen delay

// Easter Egg: button up+down turn RGB LED on
// I use it as a simple don't disturb sign
#define KICKSTARTER_RGB_LED // Many Kickstarter units were shipped with the RGB LED incorrectly installed
#ifdef KICKSTARTER_RGB_LED 
#define LIGHTS_SIZE 3
uint8_t lights[LIGHTS_SIZE][3]={{RGB_OFF,RGB_OFF,RGB_OFF},{RGB_OFF,RGB_OFF,RGB_ON},{RGB_ON,RGB_OFF,RGB_OFF}}; // #0 must be all RGB_OFF
#else
#define LIGHTS_SIZE 4
uint8_t lights[LIGHTS_SIZE][3]={{RGB_OFF,RGB_OFF,RGB_OFF},{RGB_ON,RGB_OFF,RGB_OFF},{RGB_ON,RGB_ON,RGB_OFF}, {RGB_OFF,RGB_ON,RGB_OFF}}; // #0 must be all RGB_OFF
#endif
uint8_t light=0;

uint16_t t=0;
char strategy[100];
uint8_t nolines;
int16_t s=-1;
int16_t last=-1;
uint8_t mode=0;
uint8_t conf_pos;
uint8_t min_c_last;
uint8_t max_c_last;

void setup() {
  ab.begin();
  ab.setFrameDuration(1000/FSEC);
  beep.begin();
  ab.initRandomSeed();
  initEEPROM();
  Serial.begin(9600);
  //displayTitle();
  ab.setTextWrap(false);
}

void loop() {
  if(!ab.nextFrame()){
    return;
  }
  t++;
  beep.timer();
  ab.clear();
  ab.pollButtons();
  switch(mode) {
    case 0: // initial title screen
      displayTitle();
      if(t>=start){
        mode=1;
      }
      break;
    case 1: // display strategy
      if(ab.justReleased(UP_BUTTON) && ab.justReleased(DOWN_BUTTON)){
        // cycle RGB LED
        light++;
        if(light>=LIGHTS_SIZE){
          light=0;
        }
        core.digitalWriteRGB(lights[light][0],lights[light][1],lights[light][2]);
      }
      else if(ab.justPressed(B_BUTTON)){
        // title screen
        mode=2;
      }
      else if(ab.justReleased(LEFT_BUTTON)){
        // to conf screen
        conf_pos=0;
        min_c_last=min_c;
        max_c_last=max_c;
        mode=3;
      }
      else if((s==-1)||(t>=max)||(ab.justPressed(A_BUTTON)&&(t>=min))){
        //if (ab.justPressed(A_BUTTON)){
        //  beep.tone(beep.freq(1000),0.1*FSEC);
        //}
        t=0;
        do {
          s=random(0,NO_STRATEGIES-1);
        } while (s==last);
        last=s;
      }
      //else if (ab.justPressed(A_BUTTON)){
      //  beep.tone(beep.freq(100),0.2*FSEC);
      //}
      displayStrategy(s);
      if((t>=min) && (min<max) && (nolines<8)){
        displayInfo("A NEW - B INFO - L CONF");
      }
      break;
    case 2: // title screen
      if(ab.justReleased(B_BUTTON)){
        mode=1;
      }
      displayTitle();
      break;
    case 3: // conf screen
      if(ab.justReleased(A_BUTTON)){
        beep.tone(beep.freq(500),0.1*FSEC);
        min=times[min_c]*FSEC;
        max=times[max_c]*FSEC;
        // save parameters to EEPROM
        EEPROM.update(EEPROM_START+2,min_c);
        EEPROM.update(EEPROM_START+3,max_c);
        // return to display strategy
        mode=1;
      }
      else if(ab.justReleased(B_BUTTON)){
        // restore parameters and return to display strategy
        min_c=min_c_last;
        max_c=max_c_last;
        mode=1;
      }
      else if(ab.justReleased(UP_BUTTON)){
        // cursor up
        if(conf_pos>0){
          conf_pos=0;
        }
      }
      else if(ab.justReleased(DOWN_BUTTON)){
        // cursor down
        if(conf_pos<1){
          conf_pos=1;
        }
      }
      else if(ab.justReleased(LEFT_BUTTON)){
        // decrease value
        if(conf_pos==0){
          //min=t_dec(min);
          if(min_c>0){
            --min_c;
          }
        }
        else if(conf_pos==1){
          //max=t_dec(max);
          if(max_c>0){
            --max_c;
          }
          if(min_c>max_c){
            min_c=max_c;
          }
        }
      }
      else if(ab.justReleased(RIGHT_BUTTON)){
        // increase value
        if(conf_pos==0){
          //min=t_inc(min);
          if(min_c<(TIMES_SIZE-1)){
            ++min_c;
          }
          if(min_c>max_c){
            max_c=min_c;
          }
        }
        else if(conf_pos==1){
          //max=t_inc(max);
          if(max_c<(TIMES_SIZE-1)){
            ++max_c;
          }
        }
      }
      displayConf();
      break;
  }
  Serial.write(ab.getBuffer(), 128*64/8);
  ab.display();
}

void initEEPROM() {
  uint8_t c1=EEPROM.read(EEPROM_START);
  uint8_t c2=EEPROM.read(EEPROM_START+1);
  if (c1!='O'||c2!='B'){
    // first time - save signature & default parameters
    EEPROM.update(EEPROM_START,'O');
    EEPROM.update(EEPROM_START+1,'B');
    EEPROM.update(EEPROM_START+2,min_c);
    EEPROM.update(EEPROM_START+3,max_c);
  }
  else{
    // restore parameters
    min_c=EEPROM.read(EEPROM_START+2);
    if(min_c>(TIMES_SIZE-1)){
      min_c=TIMES_SIZE-1;
    }
    min=times[min_c]*FSEC;
    max_c=EEPROM.read(EEPROM_START+3);
    if(max_c>(TIMES_SIZE-1)){
      max_c=TIMES_SIZE-1;
    }
    max=times[max_c]*FSEC;
  }
}

int wordlen(const char * str) {
  int i=0;
  while(str[i]!=' ' && str[i]!=0 && str[i]!='\n'){
    ++i;
  }
  return(i);
}

int wrap(char * s, const int width){
  int nolines=1;
  int pos=0;
  int curlinelen=0;
  while(s[pos]!=0){
    if(s[pos]==' '){
      if(curlinelen+wordlen(&s[pos+1])>=width){
        s[pos]='\n';
        nolines++;
        curlinelen=0;
      }
    }
    curlinelen++;
    pos++;
  }
  return(nolines);
}

void displayStrategy(int s) {
  strcpy_P(strategy, (char*)pgm_read_word(&(strategies[s])));
  int lc = strlen(strategy)-1;
  if(strategy[lc]!='.' && strategy[lc]!='!' && strategy[lc]!='?') {
    strcat(strategy,".");
  }
  nolines=wrap(strategy, 21);
  ab.setCursor(0,0);    
  ab.print(strategy);
}

void displayInfo(char const *s){
  tf.setCursor(0, 60);
  tf.print(s);
}

void displayTime(int t, int x, int y) {
  if(t<60){
    // seconds
    ab.setCursor(x,y);
    if(t<10){
      ab.print("0");
      ab.setCursor(x+6,y);
    }
    ab.print(t);
    ab.setCursor(x+18,y);
    ab.print("second");
    if(t!=1){
      ab.setCursor(x+18+(6*6),y);
      ab.print("s");
    }
  }
  else {
    // minutes
    ab.setCursor(x,y);
    if(t<600){
      ab.print("0");
      ab.setCursor(x+6,y);
    }
    ab.print(t/60);
    ab.setCursor(x+18,y);
    ab.print("minute");
    if(t>60){
      ab.setCursor(x+18+(6*6),y);
      ab.print("s");
    }
  }
}

void displayConf(){
  ab.setCursor(0,0);
  ab.print("OBLIQUE PARAMETERS");
  displayTime(times[min_c],8,16);
  tf.setCursor(8,26);
  tf.print("BEFORE NEW REQUEST");
  displayTime(times[max_c],8,36);
  tf.setCursor(8,46);
  tf.print("STRATEGY EXPIRATION");
  //ab.setCursor(0,16+(20*conf_pos));
  //ab.print(">");
  ab.fillRect(0,16+(20*conf_pos),4,14);
  displayInfo("A SAVE - B CANCEL");
}

void displayTitle(){
  ab.setTextSize(2);
  ab.setCursor(23,7);
  ab.print("OBLIQUE");
  ab.setCursor(5,26);
  ab.print("STRATEGIES");
  ab.setTextSize(1);
  //ab.setCursor(7,48);
  //ab.print("Thanks to Brian Eno");
  //ab.setCursor(19,56);
  //ab.print("& Peter Schmidt");
  tf.setCursor(4,48);
  tf.print("100+ WORTHWHILE DILEMMAS");
  tf.setCursor(4,56);
  tf.print("THNX B. ENO + P. SCHMIDT");
}
