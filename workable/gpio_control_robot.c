#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#include <gpiod.h>


struct gpiod_chip *chip;
struct gpiod_line_request_config config;
struct gpiod_line_bulk lines;

typedef enum Command{
  NOP       = 0,
  //
  stack1    = 1,
  stack2    = 2,
  stack3    = 3,
  stack4    = 4,
  //
  pos0      = 5,
  pos1      = 6,
  pos2      = 7,
  pos3      = 8,
  //
  ESTOP     = 9,
  hold      = 10,
  resume    = 11,
  svon      = 12,
  svoff     = 13,
  reset     = 14,
  reserved  = 15
}Command;
int RobotCommand(struct gpiod_line_bulk* plines, Command cmd);

const unsigned int gpio_out[5] = {23, 4, 17, 27, 22};

void initGpio(void);

// typedef struct Robot{
//     struct gpiod_chip *chip;
//     struct gpiod_line_request_config config;
//     struct gpiod_line_bulk lines;
//     unsigned int gpio_out[5];
// }Robot;

int main(int argc, char* argv[]){
    if(argc > 1){
        chip = gpiod_chip_open("/dev/gpiochip4");
        if(!chip){
            perror("gpiod_chip_open");
            goto cleanup;
        }
        int err = gpiod_chip_get_lines(chip, gpio_out, 5, &lines);
        if(err){
            perror("gpiod_chip_get_lines");
            goto cleanup;
        }

        memset(&config, 0, sizeof(config));
        config.consumer = "robot_command";
        config.request_type = GPIOD_LINE_REQUEST_DIRECTION_OUTPUT;
        config.flags = 0;

        int values[5] = {0, 0, 0, 0, 0};
        // get the bulk lines setting default value to 0
        err = gpiod_line_request_bulk(&lines, &config, values);
        if(err)
        {
            perror("gpiod_line_request_bulk");
            goto cleanup;
        }

        //
        while(1){
            int cmd;
            printf("====== Command List: ======\n");
            printf(\
"NOP       = 0\n \
stack1    = 1\n \
stack2    = 2\n \
stack3    = 3\n \
stack4    = 4\n \
pos0      = 5\n \
pos1      = 6\n \
pos2      = 7\n \
pos3      = 8\n \
ESTOP     = 9\n \
hold      = 10\n \
resume    = 11\n \
svon      = 12\n \
svoff     = 13\n \
reset     = 14\n \
reserved  = 15\n >> ");
            scanf("%d", &cmd);
            if(cmd == 0)
                break;
            else{
                int ret = RobotCommand(&lines, (Command)cmd);
            }
        }
        //
        cleanup:
            gpiod_line_release_bulk(&lines);
            gpiod_chip_close(chip);
    }
    return 0;
}
//
void initGpio(void){

}
//
int RobotCommand(struct gpiod_line_bulk* plines, Command cmd){
    int ret = 0;
    int err = 0;
    // Command cmd_enum = (Command)cmd;
    switch(cmd){
        case ESTOP:{
            int values[5] = {0, 1, 0, 0, 1};
            err = gpiod_line_set_value_bulk(plines, values);
            values[0] = 1;
            err = gpiod_line_set_value_bulk(plines, values);
            break;
        }
        case hold:{
            int values[5] = {0, 1, 0, 1, 0};
            err = gpiod_line_set_value_bulk(plines, values);
            values[0] = 1;
            err = gpiod_line_set_value_bulk(plines, values);            
            break;
        }
        case resume:{
            int values[5] = {0, 1, 0, 1, 1};
            err = gpiod_line_set_value_bulk(plines, values);
            values[0] = 1;
            err = gpiod_line_set_value_bulk(plines, values);           
            break;
        }
        case svon:{
            int values[5] = {0, 1, 1, 0, 0};
            err = gpiod_line_set_value_bulk(plines, values);
            values[0] = 1;
            err = gpiod_line_set_value_bulk(plines, values);
            printf("Servo ON\n");
            break;
        }
        case svoff:{
            int values[5] = {0, 1, 1, 0, 1};
            err = gpiod_line_set_value_bulk(plines, values);
            values[0] = 1;
            err = gpiod_line_set_value_bulk(plines, values);
            printf("Servo OFF\n");            
            break;
        }
        case stack1:{
            int values[5] = {0, 0, 0, 0, 1};
            err = gpiod_line_set_value_bulk(plines, values);
            values[0] = 1;
            err = gpiod_line_set_value_bulk(plines, values);           
            break;
        }
        case  stack2:{
            int values[5] = {0, 0, 0, 1, 0};
            err = gpiod_line_set_value_bulk(plines, values);
            values[0] = 1;
            err = gpiod_line_set_value_bulk(plines, values);            
            break;
        }
        case  stack3:{
            int values[5] = {0, 0, 0, 1, 1};
            err = gpiod_line_set_value_bulk(plines, values);
            values[0] = 1;
            err = gpiod_line_set_value_bulk(plines, values);            
            break;
        }
        case  stack4:{
            int values[5] = {0, 0, 1, 0, 0};
            err = gpiod_line_set_value_bulk(plines, values);
            values[0] = 1;
            err = gpiod_line_set_value_bulk(plines, values);            
            break;
        }
        case  pos0:{
            int values[5] = {0, 0, 1, 0, 1};
            err = gpiod_line_set_value_bulk(plines, values);
            values[0] = 1;
            err = gpiod_line_set_value_bulk(plines, values);            
            break;
        }
        case  pos1:{
            int values[5] = {0, 0, 1, 1, 0};
            err = gpiod_line_set_value_bulk(plines, values);
            values[0] = 1;
            err = gpiod_line_set_value_bulk(plines, values);            
            break;
        }
        case  pos2:{
            int values[5] = {0, 0, 1, 1, 1};
            err = gpiod_line_set_value_bulk(plines, values);
            values[0] = 1;
            err = gpiod_line_set_value_bulk(plines, values);            
            break;
        }
        case  pos3:{
            int values[5] = {0, 1, 0, 0, 0};
            err = gpiod_line_set_value_bulk(plines, values);
            values[0] = 1;
            err = gpiod_line_set_value_bulk(plines, values);            
            break;
        }
        case  reset:{
            int values[5] = {0, 1, 1, 1, 0};
            err = gpiod_line_set_value_bulk(plines, values);
            values[0] = 1;
            err = gpiod_line_set_value_bulk(plines, values);            
            break;
        }
        case  NOP:{
            ret = -1;
            break;
        }
    }
    if(err){
        perror("gpiod_line_set_value_bulk");
        ret = -1;
    }
    return ret;
}