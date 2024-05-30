#ifndef RCK100_ROBOT
#define RCK100_ROBOT

#include"A1_16.h"

// reference angle value(servo position counts)
#define REF_ANGLE_1       512
#define REF_ANGLE_2       480
#define REF_ANGLE_3       428
#define REF_ANGLE_4       588
#define REF_ANGLE_5       790
#define REF_ANGLE_6       707

#define M_PI                 3.14159
#define DEG2POS(degree)      ((degree)*1024/330)
#define RAD2DEG(radian)     ((radian)*180.0f/PI)
#define DEG2RAD(degree)     ((degree)*PI/180.0f)
#define RAD2COUNT(radian)   ((radian)*177.791f)   // radian*(180/PI)*(1024/330)
#define DEG2COUNT(radian)   ((radian)*3.103f)   // degree*(1024/330)

#define TEACH_POINT_SIZE 20 // 100 Hz * X sec.
#define TEACH_MODE_PTP 0
#define TEACH_MODE_STREAM 1
//
enum class Status : unsigned short{
  disabled = 0,
  sready = 1, // servo ready
  busy = 2,
  INP = 3,  // in position
  hold = 4
};
//
class Robot{
	private:
    bool estop_alarm;
    Status status;
		int robot_position_now[6];
		bool servo_on;
		unsigned int set_point[10][TEACH_POINT_SIZE][6];
    int holding_index[2];
    int ref_joint_angle_count[6];
    double DH_a[5];
    double DH_alpha[5];
    double DH_d[5];
    double DH_theta[5];
	public:
    bool trajectory_ready;
    unsigned char teach_mode;
    void EmgStop(void);
    void Hold(void);
    void Resume(void);
    void Reset(void);
    bool isHolded(void);
		void Init(void);
		void ReadRobotPosition(int* pos, bool disp);
		void ServoON(void);
		void ServoOFF(void);
		bool isServoON(void);
		void SetRobotPosition(unsigned char play_time, unsigned int* pos);
		void SetPoint(int ptp_index);
		void StartTrajectory(int ptp_index, unsigned int step_time);
    void FK_calculate(double* joint_angle , double* xyz_pos);
    void Homo_trans(double theta, int n, double T[]);
    void TeachMode(unsigned int point_number);
    void MatrixMuliply(double a[], double b[], int a_row, int a_col, int b_col, double c[]);
    Status getStatus(void);
};
//

#endif
