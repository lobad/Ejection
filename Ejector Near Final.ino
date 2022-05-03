#include <EEPROM.h>
#include <Keypad.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>

const int IN1 = 14;  // INPUTS
const int IN2 = 15;
const int IN3 = 16;
const int IN4 = 17;
const int STRT = 19;  // START CYCLE
const int STP = 18;   // STOP CYCLE
const int AIR1 = 10;  // RELAY CHANNELS
const int AIR2 = 11;
const int RLY1 = 12;
const int RLY2 = 13;
const int CYCSTRT = 23;  // START MACHINE CYCLE
const int ESTOP = 22;

const long NEXT = -1;
const long HOME = -2;
const long CLEAR = -3;

bool updateScreen = true;
bool readNum = false;
bool openConfig = false;
bool eStop = false;
bool RUN = false;
bool WAIT = false;
bool configSet = false;
bool oSet = false;
bool obSet = false;
bool o2Set = false;
bool o2bSet = false;
bool noInput = false;
bool noOutput = false;

bool iHIGH = false;
bool ibHIGH = false;
bool i2HIGH = false;
bool i2bHIGH = false;

LiquidCrystal_I2C lcd(0x27, 20, 4);

const byte ROWS = 4;
const byte COLS = 4;

char hexaKeys[ROWS][COLS] = {{'D', '#', '0', '*'},
                             {'C', '9', '8', '7'},
                             {'B', '6', '5', '4'},
                             {'A', '3', '2', '1'}};

byte rowPins[ROWS] = {9, 8, 7, 6};
byte colPins[COLS] = {5, 4, 3, 2};

Keypad customKeypad =
    Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);

// UI
long InA, InAb, InB, InBb = 0;
long delTimeA, delTimeB = 0;
long onTimeA, onTimeB = 0;
long Repeat = 0;
long OutA, OutAb, OutB, OutBb = 0;
long Cycles = 0;
// keypad input
int key;
int isnum;
// Inputs
const long INPUTS[5]{0, IN1, IN2, IN3, IN4};
const long OUTPUTS[5]{0, AIR1, AIR2, RLY1, RLY2};
// logic
long i, ib, i2, i2b = 0;  // inputs
long d, d2 = 0;           // delay
long t, t2 = 0;           // time
long o, ob, o2, o2b = 0;  // outputs
long r = 0;               // repeats set
long c = 0;               // Cycles ran

void setup() {
  Serial.begin(9600);
  Serial.println("Connected");

  customKeypad.setHoldTime(1000);
  customKeypad.setDebounceTime(25);

  lcd.init();
  lcd.clear();
  lcd.backlight();

  pinMode(IN1, INPUT);
  pinMode(IN2, INPUT);
  pinMode(IN3, INPUT);
  pinMode(IN4, INPUT);
  pinMode(STRT, INPUT);
  pinMode(STP, INPUT);
  pinMode(AIR1, OUTPUT);
  pinMode(AIR2, OUTPUT);
  pinMode(RLY1, OUTPUT);
  pinMode(RLY2, OUTPUT);
  pinMode(CYCSTRT, OUTPUT);

  digitalWrite(AIR1, HIGH);
  digitalWrite(AIR2, HIGH);
  digitalWrite(RLY1, HIGH);
  digitalWrite(RLY2, HIGH);

  lastConfig();
}

void runCycle() {
  unsigned long deadTimeA = 0;
  unsigned long deadTimeB = 0;
  unsigned long deadDelayA = 0;
  unsigned long deadDelayB = 0;
  
  lcdOptions();
  
  Cycles = 0;
  iHIGH = false;
  ibHIGH = false;
  i2HIGH = false;
  i2bHIGH = false;

  if (OutA == 0 && OutB == 0 && OutAb == 0 && OutBb == 0) {
    noOutput = true;
  }
  if (InA == 0 && InB == 0 && InAb == 0 && InBb == 0) {
    noInput = true;
  }
  if (OutAb > 0) {
    obSet = true;
  }
  if (OutBb > 0) {
    o2bSet = true;
  }
  if (OutA > 0) {
    oSet = true;
  }
  if (OutB > 0) {
    o2Set = true;
  }

  Serial.println("Starting Initial Machine Cycle");
  digitalWrite(CYCSTRT, HIGH);
  delay(50);
  digitalWrite(CYCSTRT, LOW);
  lcd.clear();
  lcd.setCursor(4, 1);
  lcd.print("Cycle Start");
  delay(600);
  Cycles = Cycles + 1;
  WAIT = true;
  lcdOptions();
  Serial.println("Waiting for Input");

  do {
    const unsigned long ms = millis();

    if (InA > 0 && digitalRead(i) == HIGH) {
      WAIT = false;
      iHIGH = true;
      Serial.println("I HIGH");
      lcdOptions();
    }
  
    if (InAb > 0 && digitalRead(ib) == HIGH) {
      WAIT = false;
      ibHIGH = true;
      Serial.println("IB HIGH");
      lcdOptions();
    }

    if (deadDelayA == 0 &&(InA == 0 || iHIGH == true) && (InAb == 0 || ibHIGH == true)) {
      deadDelayA = ms + d*100;
      Serial.println("Setting Delay A");
    }

    lcd.blink();
    lcd.setCursor(15, 1);
    Serial.println("Channel A Waiting");

    if (deadTimeA == 0 && deadDelayA > 0 && ms >= deadDelayA) {
        Serial.println("Channel A closing");
        if (oSet == true) {
          digitalWrite(o, LOW);
        }
        if (obSet == true) {
          digitalWrite(ob, LOW);
        }
        deadTimeA = ms + t*100;
    }

    const bool deadTimeAHit = deadTimeA > 0 && ms >= deadTimeA;
    if (deadTimeAHit) {
        Serial.println("Channel A opening");
        if (oSet == true) {
          digitalWrite(o, HIGH);
        }
        if (obSet == true) {
          digitalWrite(ob, HIGH);
        }
    }

    if (InB > 0 && digitalRead(i2) == HIGH) {
      WAIT = false;
      i2HIGH = true;
      Serial.println("I2 HIGH");
      lcdOptions();
    }
  
    if (InBb > 0 && digitalRead(i2b) == HIGH) {
      WAIT = false;
      i2bHIGH = true;
      Serial.println("I2B HIGH");
      lcdOptions();
    }

    if (deadDelayB == 0 && (InB == 0 || i2HIGH == true) && (InBb == 0 || i2bHIGH == true)) {
      Serial.println("Setting Delay B");
      deadDelayB = ms + d2*100;
    }

    lcd.blink();
    lcd.setCursor(17, 1);
    Serial.println("Channel B Waiting");

    if (deadTimeB == 0 && deadDelayB > 0 && ms >= deadDelayB) {
        Serial.println("Channel B closing");
        if (o2Set == true) {
          digitalWrite(o2, LOW);
        }
        if (o2bSet == true) {
          digitalWrite(o2b, LOW);
        }
        deadTimeB = ms + t2*100;
    }

    const bool deadTimeBHit = deadTimeB > 0 && ms >= deadTimeB;
    if (deadTimeBHit) {
        Serial.println("Channel B opening");      
        if (o2Set == true) {
          digitalWrite(o2, HIGH);
        }
        if (o2bSet == true) {
          digitalWrite(o2b, HIGH);
        }
    }

    const bool wasASet = InA > 0 || InAb > 0;
    const bool wasBSet = InB > 0 || InBb > 0;
    if ((wasASet == false || deadTimeAHit == true) && (wasBSet == false || deadTimeBHit == true)) {
      Serial.println("Starting Machine Cycle");
      digitalWrite(CYCSTRT, LOW);
      delay(50);
      digitalWrite(CYCSTRT, HIGH);
      lcd.clear();
      lcd.setCursor(4, 1);
      lcd.print("Cycle Start");
      delay(600);
      Cycles = Cycles + 1;
      WAIT = true;
      iHIGH = false;
      ibHIGH = false;
      i2HIGH = false;
      i2bHIGH = false;
      deadDelayA = 0;
      deadDelayB = 0;
      deadTimeA = 0;
      deadTimeB = 0;
      lcdOptions();
    }

    if (digitalRead(STP) == HIGH || eStop == true) {
      Serial.println("ABORTING CYCLE");
      lcd.clear();
      lcd.setCursor(2, 3);
      lcd.print("ABORTING");
      delay(600);
      closeCycle();
      RUN = false;
      lcdOptions();
    }

    if (noInput == true) {
      Serial.println("No Input Set");
      lcd.clear();
      lcd.setCursor(2, 3);
      lcd.print("NO INPUT SET");
      delay(600);
      closeCycle();
      RUN = false;
      lcdOptions();
      break;
    }

    if (noOutput == true) {
      Serial.println("No Output Set");
      lcd.clear();
      lcd.setCursor(2, 3);
      lcd.print("NO  OUTPUT SET");
      delay(600);
      closeCycle();
      RUN = false;
      lcdOptions();
      break;
    }

    if (digitalRead(ESTOP) == HIGH){
      eStop = true;
      lcd.clear();
      lcd.setCursor(1, 1);
      lcd.print("eStop On!");
      lcd.setCursor(2, 2);
      lcd.print("ABORTING");
      delay(600);
      Serial.println("eSTOP IS ON... ABORTING");
      RUN = false;
      closeCycle();
      lcdOptions();
    }

    if (Cycles >= r) {
      closeCycle();
      Serial.println("Cycle Completed");
      RUN = false;
      closeCycle();
      lcdOptions();
      break;
    }

  } while (configSet == true && RUN == true && eStop == false);
}

void closeCycle(){

  WAIT = false;
  obSet = false;
  o2Set = false;
  o2bSet = false;
  oSet = false;
  iHIGH = false;
  ibHIGH = false;
  i2HIGH = false;
  i2bHIGH = false;
  digitalWrite(o, HIGH);
  digitalWrite(ob, HIGH);
  digitalWrite(o2, HIGH);
  digitalWrite(o2b, HIGH);
}

void setCycle() {
  bool GO = false;

  while (GO == false) {
    Serial.println("WAITING FOR INPUT");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("PRESS START TO RUN");
    lcd.setCursor(0, 2);
    lcd.print("PRESS STOP TO ABORT");
    delay(1000);

    if (digitalRead(STRT) == HIGH) {
      delay(10);
      GO = true;
      Serial.println("CONTINUING WITH CYCYLE");

      if (RUN == true && eStop == false) {
        i = INPUTS[InA];
        ib = INPUTS[InAb];
        i2 = INPUTS[InB];
        i2b = INPUTS[InBb];
        Serial.println("INPUTS SET TO:");
        Serial.println(i);
        Serial.println(ib);
        Serial.println(i2);
        Serial.println(i2b);

        o = OUTPUTS[OutA];
        ob = OUTPUTS[OutAb];
        o2 = OUTPUTS[OutB];
        o2b = OUTPUTS[OutBb];
        Serial.println("OUTPUTS SET TO:");
        Serial.println(o);
        Serial.println(ob);
        Serial.println(o2);
        Serial.println(o2b);

        d = delTimeA;
        d2 = delTimeB;
        Serial.println("DELAYS SET TO:");
        Serial.println(d);
        Serial.println(d2);

        t = onTimeA;
        t2 = onTimeB;
        Serial.println("TIMER SET TO:");
        Serial.println(t);
        Serial.println(t2);

        r = Repeat;
        Serial.println("REPEATS SET TO:");
        Serial.println(r);

        Serial.println("CYCLE STARTING");
        configSet = true;
        runCycle();
      }
    }
     if (digitalRead(STP) == HIGH && RUN == true) {
        Serial.println("ABORTING CYCLE");
        lcd.clear();
        lcd.setCursor(2, 2);
        lcd.print("ABORTING");
        delay(600);
        RUN = false;
        lcdOptions();
        break;
      }

      if (digitalRead(ESTOP) == HIGH && RUN == true) {
        lcd.clear();
        lcd.setCursor(1, 1);
        lcd.print("eStop On!");
        lcd.setCursor(2, 2);
        lcd.print("ABORTING");
        delay(600);
        Serial.println("eSTOP IS ON... ABORTING");
        RUN = false;
        lcdOptions();
        break;
      }
  }
}

void configSelect() {
  char key = 0;

  do {
    key = customKeypad.waitForKey();
    lcd.clear();
    lcd.setCursor(2, 0);
    lcd.print("CONFIG SELECTION");
    delay(50);
    lcd.setCursor(6, 1);
    lcd.print("Channel:");
    delay(50);
    lcd.setCursor(6, 2);
    lcd.print("A  or  B");
    delay(250);

    if (key == 'A') {
      ejectorConfigA();
    }
    if (key == 'B') {
      ejectorConfigB();
    }
    if (key == 'D') {
      lcdOptions();
    }
  } while (key != 'D');
}

long readNumber() {
  Serial.println("READING INPUT");
  long entry = 0;
  char key = 0;
  lcd.blink();
  lcd.cursor();
  lcd.setCursor(8, 3);
  while (key != '#') {
    key = customKeypad.waitForKey();
    if (key >= '0' && key <= '9') {
      entry = entry * 10 + key - '0';
      Serial.println(entry);
      lcd.blink();
      lcd.cursor();
      lcd.setCursor(8, 3);
      lcd.print(entry);
    }
  }
  lcd.noBlink();
  lcd.noCursor();
  Serial.println("SETPOINT SET TO");
  Serial.println(entry);
  return entry;
}

long readConfig() {
  long input = 0;
  char key = 0;
  while (key != 'C' && key != 'D') {
    key = customKeypad.waitForKey();
    switch (key) {
      case 'A': {
        input = readNumber();
        break;
      }
      case 'B': {
        lcd.clear();
        lcd.setCursor(8, 3);
        lcd.print("SET");
        delay(600);
        return input;
      }
      case 'C': {
        lcd.clear();
        lcd.setCursor(8, 3);
        lcd.print("NEXT");
        delay(600);
        return NEXT;
      }
      case 'D': {
        lcd.clear();
        lcd.setCursor(8, 3);
        lcd.print("HOME");
        delay(600);
        return HOME;
      }
      case '*': {
        lcd.clear();
        lcd.print("CLEAR");
        delay(600);
        return CLEAR;
      }
    }
  }

  return HOME;
}

void lastConfig() {
  EEPROM.get(0, InA);
  EEPROM.get(4, InB);
  EEPROM.get(8, delTimeA);
  EEPROM.get(12, delTimeB);
  EEPROM.get(16, onTimeA);
  EEPROM.get(20, onTimeB);
  EEPROM.get(24, OutA);
  EEPROM.get(28, OutB);
  EEPROM.get(32, InAb);
  EEPROM.get(36, InBb);
  EEPROM.get(40, OutAb);
  EEPROM.get(44, OutBb);
  EEPROM.get(48, Repeat);
}

void ejectorConfigA() {
  long setPoint = 0;

  do {
    setPoint = 0;
    Serial.println("at_InA_Config");
    lcd.clear();
    lcd.setCursor(6, 0);
    lcd.print("1st Input");
    lcd.setCursor(3, 1);
    lcd.print("In A");
    lcd.setCursor(8, 1);
    lcd.print(InA);
    lcd.setCursor(1, 3);
    lcd.print("Input");
    lcd.setCursor(8, 3);
    setPoint = readConfig();
    lcd.print(setPoint);

    if (setPoint >= 0) {
      if (setPoint > 4) {
        lcd.clear();
        lcd.setCursor(5, 2);
        lcd.print("BAD INPUT");
        delay(600);
        lcd.setCursor(0, 3);
        lcd.print("MUST BE LESS THAN 4");
        delay(800);
        continue;
      }

      InA = setPoint;
      EEPROM.put(0, InA);
    } else if (setPoint == HOME) {
      return;
    }
  } while (setPoint != NEXT);

  do {
    setPoint = 0;
    Serial.println("at_InAb_Config");
    lcd.clear();
    lcd.setCursor(6, 0);
    lcd.print("2nd Input");
    lcd.setCursor(3, 1);
    lcd.print("In A");
    lcd.setCursor(8, 1);
    lcd.print(InAb);
    lcd.setCursor(1, 3);
    lcd.print("Input");
    lcd.setCursor(8, 3);
    setPoint = readConfig();
    lcd.print(setPoint);

    if (setPoint >= 0) {
      if (setPoint > 4) {
        lcd.clear();
        lcd.setCursor(5, 2);
        lcd.print("BAD INPUT");
        delay(600);
        lcd.setCursor(0, 3);
        lcd.print("MUST BE LESS THAN 4");
        delay(800);
        continue;
      }

      InAb = setPoint;
      EEPROM.put(32, InAb);
    } else if (setPoint == HOME) {
      return;
    }
  } while (setPoint != NEXT);

  do {
    setPoint = 0;
    Serial.println("at_delTimeA_Config");
    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print("Delay A");
    lcd.setCursor(8, 1);
    lcd.print(delTimeA);
    lcd.setCursor(1, 3);
    lcd.print("Input");
    lcd.setCursor(8, 3);
    lcd.print(setPoint);
    setPoint = readConfig();
    if (setPoint >= 0) {
      delTimeA = setPoint;
      EEPROM.put(8, delTimeA);
    } else if (setPoint == HOME) {
      return;
    }
  } while (setPoint != NEXT);

  do {
    setPoint = 0;
    Serial.println("at_onTimeA_Config");
    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print("Time A");
    lcd.setCursor(8, 1);
    lcd.print(onTimeA);
    lcd.setCursor(1, 3);
    lcd.print("Input");
    lcd.setCursor(8, 3);
    lcd.print(setPoint);
    setPoint = readConfig();
    if (setPoint >= 0) {
      onTimeA = setPoint;
      EEPROM.put(16, onTimeA);
    } else if (setPoint == HOME) {
      return;
    }
  } while (setPoint != NEXT);

  do {
    setPoint = 0;
    Serial.println("at_OutA_Config");
    lcd.clear();
    lcd.setCursor(6, 0);
    lcd.print("1st Output");
    lcd.setCursor(0, 1);
    lcd.print("Out A");
    lcd.setCursor(8, 1);
    lcd.print(OutA);
    lcd.setCursor(1, 3);
    lcd.print("Input");
    lcd.setCursor(8, 3);
    lcd.print(setPoint);
    setPoint = readConfig();
    if (setPoint >= 0) {
      if (setPoint > 4) {
        lcd.clear();
        lcd.setCursor(5, 2);
        lcd.print("BAD INPUT");
        delay(600);
        lcd.setCursor(0, 3);
        lcd.print("MUST BE LESS THAN 4");
        delay(800);
        continue;
      }
      OutA = setPoint;
      EEPROM.put(24, OutA);
    } else if (setPoint == HOME) {
      return;
    }
  } while (setPoint != NEXT);

  do {
    setPoint = 0;
    Serial.println("at_OutAb_Config");
    lcd.clear();
    lcd.setCursor(6, 0);
    lcd.print("2nd Output");
    lcd.setCursor(0, 1);
    lcd.print("Out B");
    lcd.setCursor(8, 1);
    lcd.print(OutAb);
    lcd.setCursor(1, 3);
    lcd.print("Input");
    lcd.setCursor(8, 3);
    lcd.print(setPoint);
    setPoint = readConfig();
    if (setPoint >= 0) {
      if (setPoint > 4) {
        lcd.clear();
        lcd.setCursor(5, 2);
        lcd.print("BAD INPUT");
        delay(600);
        lcd.setCursor(0, 3);
        lcd.print("MUST BE LESS THAN 4");
        delay(800);
        continue;
      }
      OutAb = setPoint;
      EEPROM.put(40, OutAb);
    } else if (setPoint == HOME) {
      return;
    }
  } while (setPoint != NEXT);

  do {
    setPoint = 0;
    Serial.println("at_Repeat_Config");
    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print("Repeats");
    lcd.setCursor(8, 1);
    lcd.print(Repeat);
    lcd.setCursor(1, 3);
    lcd.print("Input");
    lcd.setCursor(8, 3);
    lcd.print(setPoint);
    setPoint = readConfig();
    if (setPoint >= 0) {
      Repeat = setPoint;
      EEPROM.put(48, Repeat);
    } else if (setPoint == HOME) {
      return;
    }
  } while (setPoint != NEXT);
}

void ejectorConfigB() {
  long setPoint = 0;
  do {
    setPoint = 0;
    Serial.println("at_InB_Config");
    lcd.clear();
    lcd.setCursor(6, 0);
    lcd.print("1st Input");
    lcd.setCursor(3, 1);
    lcd.print("In B");
    lcd.setCursor(8, 1);
    lcd.print(InB);
    lcd.setCursor(1, 3);
    lcd.print("Input");
    lcd.setCursor(8, 3);
    lcd.print(setPoint);
    setPoint = readConfig();
    if (setPoint >= 0) {
      if (setPoint > 4) {
        lcd.clear();
        lcd.setCursor(5, 2);
        lcd.print("BAD INPUT");
        delay(600);
        lcd.setCursor(0, 3);
        lcd.print("MUST BE LESS THAN 4");
        delay(800);
        continue;
      }
      InB = setPoint;
      EEPROM.put(4, InB);
    } else if (setPoint == HOME) {
      return;
    }
  } while (setPoint != NEXT);

  do {
    setPoint = 0;
    Serial.println("at_InBb_Config");
    lcd.clear();
    lcd.setCursor(6, 0);
    lcd.print("2nd Input");
    lcd.setCursor(3, 1);
    lcd.print("In B");
    lcd.setCursor(8, 1);
    lcd.print(InBb);
    lcd.setCursor(1, 3);
    lcd.print("Input");
    lcd.setCursor(8, 3);
    lcd.print(setPoint);
    setPoint = readConfig();
    if (setPoint >= 0) {
      if (setPoint > 4) {
        lcd.clear();
        lcd.setCursor(5, 2);
        lcd.print("BAD INPUT");
        delay(600);
        lcd.setCursor(0, 3);
        lcd.print("MUST BE LESS THAN 4");
        delay(800);
        continue;
      }
      InBb = setPoint;
      EEPROM.put(36, InBb);
    } else if (setPoint == HOME) {
      return;
    }
  } while (setPoint != NEXT);

  do {
    setPoint = 0;
    Serial.println("at_delTimeB_Config");
    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print("Delay B");
    lcd.setCursor(8, 1);
    lcd.print(delTimeB);
    lcd.setCursor(1, 3);
    lcd.print("Input");
    lcd.setCursor(8, 3);
    lcd.print(setPoint);
    setPoint = readConfig();
    if (setPoint >= 0) {
      delTimeB = setPoint;
      EEPROM.put(12, delTimeB);
    } else if (setPoint == HOME) {
      return;
    }
  } while (setPoint != NEXT);

  do {
    setPoint = 0;
    Serial.println("at_onTimeB_Config");
    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print("Time B");
    lcd.setCursor(8, 1);
    lcd.print(onTimeB);
    lcd.setCursor(1, 3);
    lcd.print("Input");
    lcd.setCursor(8, 3);
    lcd.print(setPoint);
    setPoint = readConfig();
    if (setPoint >= 0) {
      onTimeB = setPoint;
      EEPROM.put(20, onTimeB);
    } else if (setPoint == HOME) {
      return;
    }
  } while (setPoint != NEXT);

  do {
    setPoint = 0;
    Serial.println("at_OutB_Config");
    lcd.clear();
    lcd.setCursor(6, 0);
    lcd.print("1st Output");
    lcd.setCursor(0, 1);
    lcd.print("Out B");
    lcd.setCursor(8, 1);
    lcd.print(OutB);
    lcd.setCursor(1, 3);
    lcd.print("Input");
    lcd.setCursor(8, 3);
    lcd.print(setPoint);
    setPoint = readConfig();
    if (setPoint >= 0) {
      if (setPoint > 4) {
        lcd.clear();
        lcd.setCursor(5, 2);
        lcd.print("BAD INPUT");
        delay(600);
        lcd.setCursor(0, 3);
        lcd.print("MUST BE LESS THAN 4");
        delay(800);
        continue;
      }
      OutB = setPoint;
      EEPROM.put(28, OutB);
    } else if (setPoint == HOME) {
      return;
    }
  } while (setPoint != NEXT);

  do {
    setPoint = 0;
    Serial.println("at_OutBb_Config");
    lcd.clear();
    lcd.setCursor(6, 0);
    lcd.print("2nd Input");
    lcd.setCursor(0, 1);
    lcd.print("Out B");
    lcd.setCursor(8, 1);
    lcd.print(OutBb);
    lcd.setCursor(1, 3);
    lcd.print("Input");
    lcd.setCursor(8, 3);
    lcd.print(setPoint);
    setPoint = readConfig();
    if (setPoint >= 0) {
      if (setPoint > 4) {
        lcd.clear();
        lcd.setCursor(5, 2);
        lcd.print("BAD INPUT");
        delay(600);
        lcd.setCursor(0, 3);
        lcd.print("MUST BE LESS THAN 4");
        delay(800);
        continue;
      }
      OutBb = setPoint;
      EEPROM.put(44, OutBb);
    } else if (setPoint == HOME) {
      return;
    }
  } while (setPoint != NEXT);
}

void lcdOptions() {
  lcd.clear();

  // input
  lcd.setCursor(0, 0);
  lcd.print("In");
  lcd.setCursor(4, 0);
  lcd.print(InA);
  lcd.setCursor(5, 0);
  lcd.print("|");
  lcd.setCursor(6, 0);
  lcd.print(InAb);
  lcd.setCursor(9, 0);
  lcd.print(InB);
  lcd.setCursor(10, 0);
  lcd.print("|");
  lcd.setCursor(11, 0);
  lcd.print(InBb);

  // delay
  lcd.setCursor(0, 1);
  lcd.print("Del");
  lcd.setCursor(4, 1);
  lcd.print(delTimeA);
  lcd.setCursor(9, 1);
  lcd.print(delTimeB);

  // time
  lcd.setCursor(0, 2);
  lcd.print("Tme");
  lcd.setCursor(4, 2);
  lcd.print(onTimeA);
  lcd.setCursor(9, 2);
  lcd.print(onTimeB);

  // Out
  lcd.setCursor(0, 3);
  lcd.print("Out");
  lcd.setCursor(4, 3);
  lcd.print(OutA);
  lcd.setCursor(5, 3);
  lcd.print("|");
  lcd.setCursor(6, 3);
  lcd.print(OutAb);
  lcd.setCursor(9, 3);
  lcd.print(OutB);
  lcd.setCursor(10, 3);
  lcd.print("|");
  lcd.setCursor(11, 3);
  lcd.print(OutBb);

  // repeat
  lcd.setCursor(16, 3);
  lcd.print("R");
  lcd.setCursor(17, 3);
  lcd.print(Repeat);

  // Cycles
  lcd.setCursor(16, 0);
  lcd.print("C");
  lcd.setCursor(17, 0);
  lcd.print(Cycles);

  // Running
  if (RUN == true) {
    lcd.setCursor(16, 1);
    lcd.print("RUN");
  }
  // Machine Running
  if (WAIT == true) {
    lcd.setCursor(16, 2);
    lcd.print("WAIT");
  }
  // EStop
  if (eStop == true) {
    WAIT = false;
    lcd.setCursor(15, 2);
    lcd.print("ESTOP");
  }
  delay(50);
}

void inputCheck() {
  if (digitalRead(STRT) == HIGH) {
    lcd.clear();
    Serial.println("start");
  }

  if (digitalRead(STP) == HIGH) {
    lcd.clear();
    Serial.println("stop");
  }

  if (digitalRead(IN1) == HIGH) {
    lcd.clear();
    Serial.println("1");
    digitalWrite(AIR1, LOW);
    delay(600);
    digitalWrite(AIR1, HIGH);
  }

  if (digitalRead(IN2) == HIGH) {
    lcd.clear();
    Serial.println("2");
    digitalWrite(AIR2, LOW);
    delay(600);
    digitalWrite(AIR2, HIGH);
  }

  if (digitalRead(IN3) == HIGH) {
    lcd.clear();
    Serial.println("3");
    digitalWrite(RLY1, LOW);
    delay(600);
    digitalWrite(RLY1, HIGH);
  }

  if (digitalRead(IN4) == HIGH) {
    lcd.clear();
    Serial.println("4");
    digitalWrite(RLY2, LOW);
    delay(600);
    digitalWrite(RLY2, HIGH);
  
  }
  if (digitalRead(ESTOP) == HIGH) {
    lcd.clear();
    Serial.println("ESTOP");
  }
  
}

void loop() {
  key = customKeypad.getKey();
  lcd.noBlink();

  //inputCheck();
  lcdOptions();
  delay(200);
  if (digitalRead(ESTOP) == HIGH) {
      eStop = true;
      Serial.println("EStop is Activated");
  }
   if (digitalRead(ESTOP) == LOW) {
      eStop = false;
  } 

  if (digitalRead(STRT) == HIGH) {
    delay(10);
    RUN = true;
    setCycle();
  }

  if (key == 'C') {
    Serial.println("gotoConfig");
    configSelect();
  }
}
