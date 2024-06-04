#include"rck100_robot.h"



void Robot::Init(void){
	A1_16_Ini (115200);
	this->Reset();
}
//
void Robot::Reset(void){
  servo_on = false;
  teach_mode = TEACH_MODE_PTP;
	trajectory_ready = false;
  estop_alarm = false;
  teach_index = 0;
 ref_joint_angle_count[0] = 512;
 ref_joint_angle_count[1] = 480;
 ref_joint_angle_count[2] = 428;
 ref_joint_angle_count[3] = 588;
 ref_joint_angle_count[4] = 790;
 ref_joint_angle_count[5] = 707;
 
  holding_index[0] = 0;
  holding_index[1] = 0;

 double a[5] = {0.0, 22.5, 102.37, 72.2, 0.0};
 memcpy(DH_a, a, sizeof(double)*5);
 
 double alpha[5] = {0.0, -90.0, 0.0, 0.0, 90.0};
 memcpy(DH_alpha, alpha, sizeof(double)*5);
 
 double d[5] = {20.51, 0.0, 0.0, 0.0, 142.057};
 memcpy(DH_d, d, sizeof(double)*5);
 
 double theta[5] = {180.0, -90.0, -90.0, 90.0, 0.0};
 memcpy(DH_theta, theta, sizeof(double)*5);
}
//
void Robot::ReadRobotPosition(int* pos, bool disp){

  for(int i = 0; i<6; ++i)
    pos[i] = ReadPosition(i+1);
  if(disp){
    Serial.print("Read: ");
    Serial.print("\n");
    // show joint angle:
    Serial.print("Joint angle: ");
    double angle[6];
    for(int i = 0; i<6; ++i){
      angle[i] = (double)(pos[i]-ref_joint_angle_count[i])*330.0/1024.0;
      //angle[i] = (double)pos[i];
      Serial.print(angle[i]); Serial.print(",  ");
    }
    Serial.print("\n");
    /*
    // show X Y Z: FK_cauclate(double* joint_angle , double* xyz_pos);
    Serial.print("Robot  X Y Z: {");
    double xyz[3];
    FK_calculate(angle, xyz);
    
    Serial.print(xyz[0]); Serial.print(" mm,  ");
    Serial.print(xyz[1]); Serial.print(" mm,  ");
    Serial.print(xyz[2]); Serial.print(" mm}\n\n");
    */
    delay(500);
  }
}
//
void Robot::ServoON(void){
  if(!servo_on){
    unsigned int pos_now[6];
    ReadRobotPosition(pos_now, true);
    SetRobotPosition(50, pos_now);
    servo_on = true;
  }
  Serial.print("Servo ON.\n");
  delay(100);
  this->status = Status::sready;
}
//
void Robot::ResetTeach(void){
  this->teach_index = 0;
}
//
void Robot::ServoOFF(void){
  for(unsigned char i = 1; i<=6; ++i){
    A1_16_TorqueOff(i);
  }
  servo_on = false;
  Serial.print("Servo off.\n");
  delay(200);
  this->status = Status::disabled;
}
//
bool Robot::isServoON(void){
	return servo_on;
}
//
void Robot::SetRobotPosition(unsigned char play_time, unsigned int* pos){
  for(unsigned char i = 0; i<6; ++i){
    SetPositionS_JOG(i+1, play_time, pos[i]);
  }
  servo_on = true;
}
//
void Robot::SetPoint(int ptp_index, bool export_csv){
	unsigned int pos_now[6];
	
	trajectory_ready = false;
	
	ReadRobotPosition(pos_now, false);
	
	set_point[ptp_index][teach_index][0] = pos_now[0];
	set_point[ptp_index][teach_index][1] = pos_now[1];
	set_point[ptp_index][teach_index][2] = pos_now[2];
	set_point[ptp_index][teach_index][3] = pos_now[3];
	set_point[ptp_index][teach_index][4] = pos_now[4];
	set_point[ptp_index][teach_index][5] = pos_now[5];
	
  if(!export_csv){
    Serial.print("Set point[");
    Serial.print(ptp_index);
    Serial.print("][");
    Serial.print(teach_index);
    Serial.print("] = ");
    for(unsigned char i = 0; i<6; ++i){
      Serial.print((double)(pos_now[i]-ref_joint_angle_count[i])*330.0/1024.0);
      Serial.print(", ");
    }
    Serial.print("\n");
  }
  else{
    for(unsigned char i = 0; i<6; ++i){
      Serial.print((int)pos_now[i]);
      if(i < 5)
        Serial.print(",");
      else
        Serial.println();
    }
  }
	
	++teach_index;
	if(teach_index >= TEACH_POINT_SIZE){
		trajectory_ready = true;
		teach_index = 0;
	}
	delay(200);
}
//
void Robot::EmgStop(void){
  this->ServoOFF();
  Serial.println("Emergency Stop!!");
}
//
void Robot::Hold(void){
  status = Status::hold;
  Serial.println("Suspend the task.");
}
//
bool Robot::isHolded(void){
  if(status == Status::hold){
    Serial.println("Robot is suspended in the previous task, please 'Reset' the robot before start new task or resume the remained task.");
    return true;
  }
  else{
    return false;
  }
}
//
void Robot::Resume(void){
  this->StartTrajectory(holding_index[0], 500);
  Serial.println("Resume the task.");
}
//
void Robot::StartTrajectory(int ptp_index, unsigned int step_time){
  status = Status::busy;
	char str[100];
	if(1){
    SetRobotPosition(1000, &set_point[ptp_index][0][0]);
    delay(1000);
    for(unsigned int i = holding_index[1]; i<TEACH_POINT_SIZE; ++i){

      if(status == Status::disabled){
        goto T;
      }
      if(status == Status::hold){
        holding_index[0] = ptp_index;
        holding_index[1] = i-1;
        goto T;
      }
      sprintf(str, "Moving to the point[%d][%d]......\n", ptp_index, i);
      Serial.println(str);
      SetRobotPosition(300, &set_point[ptp_index][i][0]);
      delay(step_time);
		}
	}
	else{
		Serial.print("Failed: the points of trajectory haven't been set yet.\n");
	}
	Serial.println("Trajectory completed.");

  holding_index[1] = 0;
  status = Status::INP;
  T:
    __asm__("nop");
}
//
Status Robot::getStatus(void){
  return this->status;
}
//
void Robot::FK_calculate(double* joint_angle , double* xyz_pos)
{
    double T1[16] = {0};
    double T2[16] = {0};
    double T3[16] = {0};
    double T4[16] = {0};
    double T5[16] = {0};
    double T5_m[16] = {0};  
    double T02[16] = {0};
    double T03[16] = {0};
    double T04[16] = {0};
    double T05[16] = {0};  

    Homo_trans(joint_angle[0]+180.0, 1, T1);
    Homo_trans(joint_angle[1]-90.0, 2, T2);
    Homo_trans(joint_angle[2]-90.0, 3, T3);
    Homo_trans(joint_angle[3]+90.0, 4, T4);
    Homo_trans(joint_angle[4], 5, T5);
    //Homo_trans(angle_rad[5], 6, T5_m);
      
    MatrixMuliply(T1, T2, 4, 4, 4, T02);    
    MatrixMuliply(T02, T3, 4, 4, 4, T03);  
    MatrixMuliply(T03, T4, 4, 4, 4, T04);
    //MatrixMuliply(T04, T5_m, 4, 4, 4, T05);
    MatrixMuliply(T04, T5, 4, 4, 4, T05);
       
    xyz_pos[0] = T05[3];
    xyz_pos[1] = T05[7];
    xyz_pos[2] = T05[11];
}
//
void Robot::Homo_trans(double theta, int n, double T[])
{
    theta = PI * theta / 180.0;

    /*~~~~~~~~~~~~~~~~~~~~*/
    double cs = cos(theta);
    double ss = sin(theta);
    double ca = 0.0;
    double sa = 0.0;
    double A = 0.0;
    double D = 0.0;
    /*~~~~~~~~~~~~~~~~~~~~*/

    switch (n)
    {
        case 1:
            ca = cos(PI * DH_alpha[0] / 180.0);
            sa = sin(PI * DH_alpha[0] / 180.0);
            A = DH_a[0];
            D = DH_d[0];
            break;

        case 2:
            ca = cos(PI * DH_alpha[1] / 180.0);
            sa = sin(PI * DH_alpha[1] / 180.0);
            A = DH_a[1];
            D = DH_d[1];
            break;

        case 3:
            ca = cos(PI * DH_alpha[2] / 180.0);
            sa = sin(PI * DH_alpha[2] / 180.0);
            A = DH_a[2];
            D = DH_d[2];
            break;

        case 4:
            ca = cos(PI * DH_alpha[3] / 180.0);
            sa = sin(PI * DH_alpha[3] / 180.0);
            A = DH_a[3];
            D = DH_d[3];
            break;

        case 5:
            ca = cos(PI * DH_alpha[4] / 180.0);
            sa = sin(PI * DH_alpha[4] / 180.0);
            A = DH_a[4];
            D = DH_d[4];
            break;   
//        case 6:
//            ca = cos(PI * DH_alpha[4] / 180.0);
//            sa = sin(PI * DH_alpha[4] / 180.0);
//            A = DH_a[4];
//            D = (DH_d[4] - 92.057);
//            break;                
    }

    /* Modified */
    T[0] = cs;
    T[1] = -ss;
    T[2] = 0;
    T[3] = A;
    T[4] = ss * ca;
    T[5] = cs * ca;
    T[6] = -sa;
    T[7] = -sa * D;
    T[8] = ss * sa;
    T[9] = cs * sa;
    T[10] = ca;
    T[11] = ca * D;
    T[12] = 0;
    T[13] = 0;
    T[14] = 0;
    T[15] = 1;   
}

//FK matrix caculate
void Robot::MatrixMuliply(double a[], double b[], int a_row, int a_col, int b_col, double c[])
{
    for (int i = 0; i < a_row; i++)
    {
        for (int j = 0; j < b_col; j++)
        {
            c[i * b_col + j] = 0.0;
            for (int k = 0; k < a_col; k++)
                c[i * b_col + j] += a[i * a_col + k] * b[k * b_col + j];
        }
    }
}

//
void Robot::MoveToReadyPos(void){
	SetRobotPosition(400, READY_POS);
}
//
void Robot::LoadTeach(void){
    for(int j = 0; j<TEACH_POINT_SIZE; ++j)
      for(int k = 0; k<6; ++k)
        this->set_point[4][j][k] = READY_TO_PICK[j][k];

    for(int j = 0; j<TEACH_POINT_SIZE; ++j)
      for(int k = 0; k<6; ++k)
        this->set_point[0][j][k] = READY_TO_STACK1[j][k];
        
    for(int j = 0; j<TEACH_POINT_SIZE; ++j)
      for(int k = 0; k<6; ++k)
        this->set_point[1][j][k] = READY_TO_STACK2[j][k];

    for(int j = 0; j<TEACH_POINT_SIZE; ++j)
      for(int k = 0; k<6; ++k)
        this->set_point[2][j][k] = READY_TO_STACK3[j][k];
    
}
