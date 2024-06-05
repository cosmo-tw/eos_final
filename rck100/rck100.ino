#include "rck100_robot.h"
#define SW_START 3
#define SW_SET_POINT 24
#define SW_SERVO_ON 5
#define SW_SELECT_PTP 22

#define TIMER1_ISR_HZ 100
/*
  1. The GPIO pin available for EXT. interrupt: Mega, Mega2560: 2, 3, 18, 19, 20
  2. mode: 
        LOW to trigger the interrupt whenever the pin is low,

        CHANGE to trigger the interrupt whenever the pin changes value

        RISING to trigger when the pin goes from low to high,

        FALLING for when the pin goes from high to low.
*/
#define EXT_INT_PIN 2
#define EXT_INT_MODE RISING
#define BUSY_FLAG_PIN 13
int BUSY_FLAG;
#define SetBusyFlag digitalWrite(BUSY_FLAG_PIN, true); BUSY_FLAG = 1;
#define ResetBusyFlag digitalWrite(BUSY_FLAG_PIN, false); BUSY_FLAG = 0;
//
enum class Command : unsigned short {
  NOP = 0,
  //
  stack1 = 1, // ptp-0
  stack2 = 2, // ptp-1
  stack3 = 3, // ptp-2
  stack4 = 4, // ptp-3
  //
  ready_pos = 5, // 
  pos1 = 6,      // ptp-4
  pos2 = 7,      // ptp-5
  pos3 = 8,      // ptp-6
  //
  ESTOP = 9,
  hold = 10,
  resume = 11,
  svon = 12,
  svoff = 13,
  reset = 14
};
const int COMMAND_PIN[4] = { 3, 4, 5, 6 };
const int STATUS_PIN[4] = { 7, 8, 9 };
Command ReadCommand(void);
void ReturnStatus(void);
int flag;
//
Robot rb;
//
void initTimerISR(void);
void initExtInterrupt(void);
void ExtIntISR(void);
//
void setup() {
  BUSY_FLAG = 0;
  // 初始化系統：
  Serial.begin(9600);
  // 配置按鈕:
  //pinMode(SW_START, INPUT_PULLUP);
  pinMode(SW_SELECT_PTP, INPUT_PULLUP);  //
  pinMode(SW_SET_POINT, INPUT_PULLUP);
  pinMode(26, INPUT_PULLUP);
  pinMode(BUSY_FLAG_PIN, OUTPUT);
  // Assign PINs for Command:
  for (int i = 0; i < 4; ++i)
    pinMode(COMMAND_PIN[i], INPUT);
  // 初始化機械手臂 RCK100:
  rb.Init();
  // the other functions:
  pinMode(EXT_INT_PIN, INPUT);
  //
  unsigned int ref_joint_angle_count[6];
  ref_joint_angle_count[0] = 512;
  ref_joint_angle_count[1] = 480 + 50;
  ref_joint_angle_count[2] = 428 + 270;
  ref_joint_angle_count[3] = 588 - 250;
  ref_joint_angle_count[4] = 790;
  ref_joint_angle_count[5] = 707;
  //rb.SetRobotPosition(400, ref_joint_angle_count);
  rb.LoadTeach();
  digitalWrite(BUSY_FLAG_PIN, false);
}
//
void demo1(void) {
  if (digitalRead(SW_START) == false) {
    Serial.print("2\n");
    rb.StartTrajectory(0, 500);
    delay(200);
  }
  if (digitalRead(SW_SERVO_ON) == false) {
    if (rb.isServoON() == false) {
      rb.ServoON();
    } else {
      rb.ServoOFF();
    }
  }
  if (digitalRead(SW_SET_POINT) == false) {
    rb.SetPoint(0);
  }
  // 監控當前機械手臂的位置:
  int pos[6];
  //rb.ReadRobotPosition(pos, true);]-
  delay(100);
}
//
void loop() {
  initExtInterrupt();
  // --------------------------------------
  if (flag == 1) {
    Operation();
    flag = 0;
  }
  /*else if(flag == 1 && BUSY_FLAG == 1){
    ResetBusyFlag
  }*/
  // --------------------------------------
  char str[100];
  static int i = 0;
  if (digitalRead(SW_SELECT_PTP) == false) {
    sprintf(str, "Select Teach PTP-%i", i);
    Serial.println(str);
    if (++i == 10)
      i = 0;
    rb.ResetTeach();
  } else if (digitalRead(SW_SET_POINT) == false) {
    rb.SetPoint(i, true);
  }
  else if(digitalRead(26) == false){
      rb.StartTrajectory(4, 400);
      rb.StartTrajectory(0, 400);
      rb.StartTrajectory(4, 400);
      rb.StartTrajectory(1, 400);
      rb.StartTrajectory(4, 400);
      rb.StartTrajectory(2, 400);
      rb.StartTrajectory(4, 400);
      rb.StartTrajectory(3, 400);
  }
  // --------------------------------------
  delay(100);
}
//
void initTimer1(void) {
  noInterrupts();  //disable interrupt services
                   //==============================================
                   //----------------- Timer-1 //-----------------
  TCCR1A = 0;      // set entire TCCR1A register to 0
  TCCR1B = 0;      // same for TCCR1B
  TCNT1 = 0;       //initialize counter value to 0
  // set compare match register for 1hz increments
  // prescaler = 64
  OCR1A = 2499;  // = (16*10^6) / (100*64) - 1 (must be <65536), compare match register = [ 16,000,000Hz/ (prescaler * desired interrupt frequency) ] - 1
  // turn on CTC mode
  TCCR1B |= (1 << WGM12);
  // Set CS10 and CS12 bits for 64 prescaler
  TCCR1B |= (1 << CS11) | (1 << CS10);
  // enable timer compare interrupt
  TIMSK1 |= (1 << OCIE1A);
  //==============================================
  interrupts();  //enable interrupt services
}
ISR(TIMER1_COMPA_vect) {  //timer1 ISR handler
  static unsigned short point_count = 0;

  ++point_count;
}
//
void initExtInterrupt(void) {
  attachInterrupt(digitalPinToInterrupt(EXT_INT_PIN), ExtIntISR, EXT_INT_MODE);
}
//
void ExtIntISR(void) {
  //Operation();
  SetBusyFlag
  flag = 1;
  Serial.println("EXT");
}
//
Command ReadCommand(void) {
  int cmd = 0;
  for (int i = 0; i < 4; ++i) {
    if (digitalRead(COMMAND_PIN[i])) {
      cmd |= 8 >> i;
      Serial.print("1, ");
    } else {
      Serial.print("0, ");
    }
  }
  return static_cast<Command>(cmd);
}
//
Command Operation(void) {
  switch (ReadCommand()) {
    case Command::ESTOP:
      {
        Serial.println("EmgStop");
        rb.EmgStop();
        
        break;
      }
    case Command::hold:
      {
        Serial.println("Hold");
        rb.Hold();
        
        break;
      }
    case Command::resume:
      {
        Serial.println("Resume");
        rb.Resume();
        
        break;
      }
    case Command::svon:
      {
        Serial.println("Servo ON");
        rb.ServoON();
        
        break;
      }
    case Command::svoff:
      {
        Serial.println("Servo OFF");
        rb.ServoOFF();
        
        break;
      }
    case Command::stack1:
      {
        SetBusyFlag
        Serial.println("Ready to stack 1");
        if (!rb.isHolded())
          rb.StartTrajectory(0, 500);
        ResetBusyFlag
        break;
      }
    case Command::stack2:
      {
        SetBusyFlag
        Serial.println("Ready to stack 2");
        if (!rb.isHolded())
          rb.StartTrajectory(1, 500);
        ResetBusyFlag
        break;
      }
    case Command::stack3:
      {
        SetBusyFlag
        Serial.println("Ready to stack 3");
        if (!rb.isHolded())
          rb.StartTrajectory(2, 500);
        ResetBusyFlag
        break;
      }
    case Command::stack4:
      {
        SetBusyFlag
        Serial.println("Ready to stack 4");
        if (!rb.isHolded())
          rb.StartTrajectory(3, 500);
        ResetBusyFlag
        break;
      }
    case Command::ready_pos:
      {
        SetBusyFlag
        Serial.println("Ready to pick position");
        if (!rb.isHolded())
          rb.StartTrajectory(4, 500);
        ResetBusyFlag
        break;
      }
    case Command::pos1:
      {/*
        SetBusyFlag;
        Serial.println("pos 1");
        if (!rb.isHolded())
          rb.StartTrajectory(5, 500);
        ResetBusyFlag;*/
        break;
      }
    case Command::pos2:
      {/*
        SetBusyFlag;
        Serial.println("pos 2");
        if (!rb.isHolded())
          rb.StartTrajectory(6, 500);
        ResetBusyFlag;*/
        break;
      }
    case Command::pos3:
      {/*
        SetBusyFlag;
        Serial.println("pos 3");
        if (!rb.isHolded())
          rb.StartTrajectory(7, 500);
        ResetBusyFlag;*/
        break;
      }
    case Command::reset:
      {
        rb.Reset();
        break;
      }
    default:
      {

        break;
      }
  }
}
