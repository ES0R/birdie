/*  
 * 
 * Copyright © 2023 DTU,
 * Author:
 * Christian Andersen jcan@dtu.dk
 * 
 * The MIT License (MIT)  https://mit-license.org/
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 * and associated documentation files (the “Software”), to deal in the Software without restriction, 
 * including without limitation the rights to use, copy, modify, merge, publish, distribute, 
 * sublicense, and/or sell copies of the Software, and to permit persons to whom the Software 
 * is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all copies 
 * or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
 * PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE 
 * FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN 
 * THE SOFTWARE. */

#include <string>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include "mpose.h"
#include "steensy.h"
#include "uservice.h"
#include "sencoder.h"
#include "utime.h"
#include "cmotor.h"
#include "cservo.h"
#include "medge.h"
#include "cedge.h"
#include "cmixer.h"
#include "sdist.h"
#include "bplan20.h"
#include "sencoder.h"
#include "simu.h"
#include <sys/socket.h>
#include <arpa/inet.h>
// create class object
BPlan20 plan20;
//CEdge edge; //for the line following


void BPlan20::setup()
{ // ensure there is default values in ini-file
  if (not ini["plan20"].has("log"))
  { // no data yet, so generate some default values
    ini["plan20"]["log"] = "true";
    ini["plan20"]["run"] = "true";
    ini["plan20"]["print"] = "true";
  }
  // get values from ini-file
  toConsole = ini["plan20"]["print"] == "true";
  //
  if (ini["plan20"]["log"] == "true")
  { // open logfile
    std::string fn = service.logPath + "log_plan20.txt";
    logfile = fopen(fn.c_str(), "w");
    fprintf(logfile, "%% Mission plan20 logfile\n");
    fprintf(logfile, "%% 1 \tTime (sec)\n");
    fprintf(logfile, "%% 2 \tMission state\n");
    fprintf(logfile, "%% 3 \t%% Mission status (mostly for debug)\n");
    
  }
  setupDone = true;
}

BPlan20::~BPlan20()
{
  terminate();
}


void BPlan20::run()
{
  
  if (not setupDone){
    setup();
  }
    
  if (ini["plan20"]["run"] == "false"){
    return;
  }
    
    
  //
  UTime t("now");
  bool finished = false;
  bool lost = false;
  bool box = false;
  bool pick_up_upstairs_ball = false;
  int step_counter = 0;
  int encoder_ = 0;
  state = 0;
  int last_transition = 0; //BREAYTA I 0
  int encoder_target = 0;//BREYTA I 0
  dist_to_ball = 0;
  angle_to_ball = 0;
  oldstate = state;
  //
  // toLog("Plan20 started");
  //
  
  
  while (not finished and not lost and not service.stop)
  {
    
    switch (state)
    { 
      case 0: //TEST CASE
        servo.setServo(3, 1, -850, 200);
        sleep(5);
        send_command("127.0.0.1", 25005, "golf"); //LOOK HERE FIX
        state = 11111;
        break;
      
      case 11111:
        cout << dist_to_ball << endl;
        sleep(2);
        break;

      case 10000:
        
        if (imu.gyro[2] < -150){
          encoder_ = getTicks(1.40);
          encoder_target = encoder.enc[1] + encoder_;
          state = 10001;
        }
        break;
      
      case 10001:
        if (encoder.enc[1] > encoder_target){
          mixer.setVelocity(0);
          state = 10002;
        }
        
        break;
      
      case 10002:
        servo.setServo(3, 1, -80, 200);
        sleep(5);
        state = 10003;
        break;

      case 10003:
        mixer.setEdgeMode(true, 0);
        mixer.setVelocity(0.05);
        break;
      
      case 150: //Start ramp
        
        mixer.setEdgeMode(true, 0);
        mixer.setVelocity(0.4);
        encoder_ = getTicks(4);
        encoder_target = encoder.enc[1] + encoder_;
        state = 151;
        break;
      
      case 151:
        if (encoder.enc[1] > encoder_target){
          mixer.setVelocity(0.05);
          state = 1552;
        }
        break;
      
      case 1552:
        if (imu.gyro[2] < -150){
          encoder_ = getTicks(1.40);
          encoder_target = encoder.enc[1] + encoder_;
          state = 1553;
        }
        break;
      
      case 1553:
        if (encoder.enc[1] > encoder_target){
          mixer.setVelocity(0);
          state = 1554;
        }
        break;

      case 1554:
        servo.setServo(3, 1, -80, 200);
        sleep(5);
        state = 10003;
        break;
      
      case 1555:
        mixer.setEdgeMode(true, 0);
        mixer.setVelocity(0.05);
        state = 152;
        break;

      case 152:
        if (imu.acc[2] < -2.0){
          mixer.setTurnrate(0);
          mixer.setVelocity(0.05);
          state = 153;
        }
        break;
      
      case 153:
        if (medge.edgeValid){
          mixer.setEdgeMode(false, 0);
          encoder_target = encoder.enc[1] + 1000;
          state = 154;
        }
        break;

      case 154:
        if (encoder.enc[1] > encoder_target){
          mixer.setEdgeMode(true, 0);
          sleep(2);
          state = 155;
        }
        break;
      
      case 155: //End Ramp
        if (imu.gyro[2] < -100){
          
          encoder_target = encoder.enc[1] + 1000;
          last_transition = 155;
          state = 138;
        }
        break;
      
      
      case 156: //HAPPENS BEFORE RAMP
        if (encoder.enc[1] > encoder_target){
          mixer.setEdgeMode(true, 0);
          sleep(1);
          mixer.setVelocity(0.3);
          encoder_ = getTicks(9);
          encoder_target = encoder.enc[1] + encoder_;
          state = 157;
        }
        break;
      
      case 157:
        if (encoder.enc[1] > encoder_target){
          mixer.setVelocity(0);
          pose.resetPose();
          sleep(2);
          mixer.setDesiredHeading(3.14);
          sleep(2);
          state = 150;
        }

        break;



      case 121: //START STAIRS
        if (encoder.enc[1] > encoder_target){
          mixer.setEdgeMode(true, 0);
          mixer.setVelocity(0.3);
          sleep(3);
          state = 122;
        }
        
        break;
      
      case 122:
        if (imu.gyro[2] > 50){
          encoder_target = encoder.enc[1] + 1500;
          state = 123;
        }
        break;
      
      case 123:
        if (encoder.enc[1] > encoder_target){
          mixer.setVelocity(0.1);
          mixer.setEdgeMode(false, 0);
          encoder_target = encoder.enc[1] + 1250;
          if (pick_up_upstairs_ball){
            state = 100;
          } else {
            state = 124;
          }
          
        }
        break;
      
      case 124:
        if (encoder.enc[1] > encoder_target){
          mixer.setVelocity(0);
          servo.setServo(3, 1, 150, 200);
          sleep(5);
          state = 135;
        }
        break;

      case 135:
        mixer.setVelocity(0.1);
        mixer.setEdgeMode(false, 0);
        state = 136;

        break;
      
      case 136:
        if (imu.acc[2] < -0.2){
          usleep(1750000);
          step_counter ++;
        }
        if (step_counter == 10){
          state = 137;
        }
        break;
      
      case 137:
        sleep(3);
        mixer.setVelocity(0);
        sleep(2);
        servo.setServo(3, 1, -850, 200);
        sleep(5);
        mixer.setVelocity(0.1);
        
        mixer.setEdgeMode(false, 0);
        encoder_target = encoder.enc[1] + 1250;
        last_transition = 138;
        state = 138;
        break;

      case 138: //STAIRS END
        if (encoder.enc[1] > encoder_target){
          mixer.setVelocity(0);
          pose.resetPose();
          sleep(2);
          mixer.setDesiredHeading(-3.14*0.5);

          sleep(2);
          mixer.setVelocity(0.1);
          encoder_target = encoder.enc[1] + 1000;
          state = 8;

        }
        break;

      case 80: // After spinny thing put low speed to make turn
        mixer.setVelocity(0.1);
        mixer.setEdgeMode(true, 0);
        state = 81;
        break;

      case 81: //Go up to box door and turn to the left
        if (dist.dist[0] < 0.15){
          mixer.setVelocity(0);
          mixer.setTurnrate(0);
          pose.resetPose();
          sleep(2);
          mixer.setDesiredHeading(3.14*0.5);
          sleep(2);
          pose.resetPose();
          mixer.setVelocity(0.2);
          state = 82;
        }
        break;
      
      case 82: // Find new line and turn and drive to position for swing
        if (medge.edgeValid){
          mixer.setVelocity(0);
          sleep(2);
          mixer.setDesiredHeading(-3.14*0.35);
          sleep(2);
          pose.resetPose();
          mixer.setVelocity(0.2);
          encoder_target = encoder.enc[1] + 600;
          state = 83;
        }
        break;
      
      case 83: //Swing the robot to open the door. Then start driving to find line again
        if (encoder.enc[1] > encoder_target){
          mixer.setVelocity(0);
          sleep(2);
          mixer.setDesiredHeading(-3.14*0.9);
          sleep(2);
          pose.resetPose();
          mixer.setVelocity(0.075);
          state = 84;
        }
        break;

      case 84: //Find line and drive into the box
        if (medge.edgeValid){
          mixer.setEdgeMode(true, 0);
          
          state = 85;
        }
        break;

      case 85: // Use gyro to know when to check for distance
        if (imu.gyro[2] < -150){
          sleep(5);
          
          state = 86;
        }

        break;
      
      case 86: // Use distance to second door to know the distance to next checkpoint
        if (dist.dist[0] < 0.15){
          encoder_target = encoder.enc[1] + 2650;
          
          state = 87;
        }
        break;

      case 87: // Stop after opening the second door and then swing to close it and swing again to reach line again
        if (encoder.enc[1] > encoder_target){
          pose.resetPose();
          mixer.setVelocity(0);
          sleep(2);
          mixer.setDesiredHeading(3.14*0.9);
          sleep(2);
          pose.resetPose();
          sleep(2);
          mixer.setDesiredHeading(3.14*0.2);
          sleep(2);
          pose.resetPose();
          state = 88;
        }
        
        break;
      
      case 88: // Go slow up to the door to be sure it is closed, then back away
        mixer.setVelocity(0.05);
        mixer.setEdgeMode(false, 0);
        sleep(9);
        mixer.setTurnrate(0);
        mixer.setVelocity(-0.2);
        encoder_target = encoder.enc[1] - 600;
        state = 89;
        
        break;
      
      case 89: //Back away the required distance and get ready for state 90
        if (encoder.enc[1] < encoder_target){
          mixer.setVelocity(0);
          pose.resetPose();
          sleep(2);
          state = 90;
        }
        break;

      case 90: //Turn left and start driving for encoder target distance
        mixer.setDesiredHeading(3.14*0.5);
        sleep(2);
        
        mixer.setVelocity(0.1);
        pose.resetPose();
        encoder_target = encoder.enc[1] + 1000;
        state = 91;
        break;

      case 91: //Reach encoder distance and turn right and start driving again
        if (encoder.enc[1] > encoder_target){
          mixer.setVelocity(0);
          sleep(2);
          mixer.setDesiredHeading(-3.14*0.5);
          sleep(2);
          mixer.setVelocity(0);
          sleep(2);
          mixer.setVelocity(0.2);
          pose.resetPose();
          encoder_target = encoder.enc[1] + 3250;
          state = 92;
        }
        break;

      case 92: //Reach the first door to close it and then swing
        if (encoder.enc[1] > encoder_target){
          mixer.setVelocity(0);
          sleep(2);
          mixer.setDesiredHeading(-3.14*0.35);
          sleep(2);

          mixer.setVelocity(0.2);
          pose.resetPose();
          state = 93;
        }
        break;

      case 93: //Find the line and follow it in wrong direction for 3 seconds, then turn 180 and follow right to ensure that the door is closed
        if (medge.edgeValid){
          mixer.setEdgeMode(true, 0);
          sleep(3);
          mixer.setVelocity(0);
          pose.resetPose();
          sleep(2);
          mixer.setVelocity(-0.2);
          sleep(4);
          mixer.setVelocity(-0.05);
          sleep(10);
          state = 94;
        }
        break;
      
      case 94: //Back away from the door 
        
        
        pose.resetPose();
        mixer.setVelocity(0);
        sleep(2);
        box = true;
        state = 95;
        
        break;
      
      case 95://Turn around and resume to following the line until end of race and then go to state 23
        pose.resetPose();
        sleep(2);
        
        mixer.setEdgeMode(true, 0);
        mixer.setVelocity(0.4);
        sleep(2);
        state = 23;
        break;

      case 1: //Start run and follow left until circle robot ring
        
        sleep(2);
        servo.setServo(3, 1, -850, 200);
        sleep(5);
        mixer.setEdgeMode(true, 0);
        mixer.setVelocity(0.3);
        encoder_target = encoder.enc[1] + 9500;
        state = 2;
        break;
      
      case 2: //Turn to view circle robot
        if (encoder.enc[1] > encoder_target){
          mixer.setVelocity(0);
          pose.resetPose();
          sleep(2);
          mixer.setDesiredHeading(3.14*0.3);
          sleep(2);
          pose.resetPose();
          sleep(1);
          state = 3;

        }
        break;
      
      case 3: //Wait for the circle robot
        if (dist.dist[0] < 0.2){
          state = 4;
          
        }
        break;

      case 4: //Wait for the circle robot to pass and turn back
        if (dist.dist[0] > 0.5){
          mixer.setDesiredHeading(-3.14*0.2);
          sleep(1);
          
          state = 5;
        }
        break;
      
      case 5: //Fix yourself on line and pass through the intersection. 
        
        mixer.setVelocity(0.1);
        mixer.setEdgeMode(false, 0);
        
        sleep(2);
        mixer.setTurnrate(0);
        mixer.setVelocity(0.2);
        sleep(2);
        if (last_transition == 112){ // If the loop is done go to spinny thing
          encoder_target = encoder.enc[1] + 5000;
          state = 113;
        } else {
          state = 6;
        }
        
        break;
      

      case 6: // After passing intersection, latch onto line again
        mixer.setVelocity(0.3);
        mixer.setEdgeMode(false, 0);
        encoder_target = encoder.enc[1] + 4000;
        
        state = 7;
        break;

      case 7: // Stop following line to go to next line (to go full circle)

        if (encoder.enc[1] > encoder_target){
          mixer.setTurnrate(0);
          sleep(1);
          mixer.setVelocity(0.2);
          encoder_target = encoder.enc[1] + 3000;
          state = 8;
        }
        
        break;
      
      case 8: // Find the line and latch onto it

        if (medge.edgeValid && encoder.enc[1] > encoder_target){
          mixer.setEdgeMode(false, 0);
          encoder_target = encoder.enc[1] + 500;
          if (last_transition == 0){
            state = 121;
          } else if (last_transition == 138){
            state = 156;
          }else{
            state = 9;
          }
            
          }
          
      break;

      case 9: //Start following left the whole circle
        servo.setServo(3, 1, -80, 300); //delete
        sleep(5);//delete
        if (encoder.enc[1] > encoder_target){

          mixer.setEdgeMode(true, 0);
          sleep(1);
          mixer.setVelocity(0.2);
          pick_up_upstairs_ball = true;
          // encoder_target = encoder.enc[1] + 27000;
          state = 1556;
        }
        break;
      
      case 1556:
        if (dist.dist[0] < 0.17){
          encoder_ = getTicks(2.8);
          encoder_target = encoder.enc[1] + encoder_;
          sleep(5);
          servo.setServo(3, 1, 25, 300);
          
          state = 1557;
        }

        break;

      case 1557:
        if (encoder.enc[1] > encoder_target){
          servo.setServo(3, 1, 150, 400);
          state = 1558;
        }
        break;

      case 1558:
        if (imu.gyro[2] > 30){
          mixer.setVelocity(0);
          mixer.setTurnrate(0);
          servo.setServo(3, 1, 24, 200);
          sleep(2);
          state = 1559;
        }
        break;

      case 1559:
        mixer.setDesiredHeading(3.14*0.175);
        sleep(2);
        encoder_ = getTicks(0.19);
        encoder_target = encoder.enc[1] + encoder_;
        mixer.setVelocity(0.1);
        state = 1660;
        break;

      case 1660:
        if (encoder.enc[1] > encoder_target){
          mixer.setVelocity(0);
          sleep(2);
          pose.resetPose();
          sleep(2);
          mixer.setDesiredHeading(-3.14*0.05);
          sleep(2);
          mixer.setDesiredHeading(-3.14*0.1);
          sleep(2);
          mixer.setDesiredHeading(0);
          sleep(2);
          pose.resetPose();
          sleep(2);
          mixer.setVelocity(-0.1);
          state = 1661;
        }
        break;

      case 1661:
        if (medge.edgeValid){
          sleep(1);
          mixer.setVelocity(0);
          sleep(2);
          servo.setServo(3, 1, -850, 200);
          sleep(5);
          mixer.setVelocity(0.3);
          mixer.setEdgeMode(true, 0);
          encoder_target = encoder.enc[1] + 1500;
          state = 123;
        }
        break;


      case 100: 
        if (encoder.enc[1] > encoder_target){
          mixer.setVelocity(0);
          pose.resetPose();
          sleep(2);
          mixer.setDesiredHeading(3.14*0.63);
          sleep(2);
          mixer.setVelocity(0.1);
          encoder_ = getTicks(0.07);
          encoder_target = encoder.enc[1] + encoder_;
          state = 101;
        }
        
        break;


      case 101:
        if (encoder.enc[1] > encoder_target){
          mixer.setVelocity(0);
          pose.resetPose();
          sleep(2);
          servo.setServo(3, 1, 0, 200);
          sleep(6);

          state = 102;
        }
        break;

      case 102:
        mixer.setDesiredHeading(3.14*0.99);
        sleep(2);
        mixer.setVelocity(0.1);
        encoder_ = getTicks(0.68);
        
        encoder_target = encoder.enc[1] + encoder_;
        state = 103;
        break;
      
      case 103: 
        if (encoder.enc[1] > encoder_target){
          mixer.setVelocity(0);
          pose.resetPose();
          sleep(2);
          mixer.setDesiredHeading(-3.14*0.05);
          sleep(2);
          mixer.setDesiredHeading(-3.14*0.1);
          sleep(2);
          mixer.setDesiredHeading(-3.14*0.15);
          sleep(2);
          mixer.setDesiredHeading(0);
          sleep(2);
          mixer.setDesiredHeading(3.14*0.05);
          sleep(2);
          mixer.setDesiredHeading(3.14*0.1);
          sleep(2);
          mixer.setDesiredHeading(3.14*0.15);
          sleep(2);
          
          mixer.setVelocity(-0.1);
          state = 104;
        }
        break;
      
      case 104:
        if (medge.edgeValid){
          mixer.setVelocity(0);
          sleep(2);
          pose.resetPose();
          sleep(2);
          servo.setServo(3, 1, -850, 200);
          sleep(5);
          mixer.setDesiredHeading(-3.14*0.7);
          sleep(3);
          
          encoder_ = getTicks(9);
          mixer.setVelocity(0.05);
          mixer.setEdgeMode(true, 0);
          encoder_target = encoder.enc[1] + encoder_;
          
          sleep(4);
          mixer.setVelocity(0.3);
          state = 110;
        }
        break;

      case 110: // Slow down to make harsh right near beginning
        if (encoder.enc[1] > encoder_target){
          mixer.setVelocity(0.1);
          mixer.setEdgeMode(false, 0);
          state = 111;
        }
        break;

      case 111: // Use gyro to start encoder count from harsh turn
        
        if (imu.gyro[2] > 150){
          encoder_target = encoder.enc[1] + 1650;
          state = 112;
        }
        break;

      case 112: //Go to position to view circle robot again and return to state 2. 
        if (encoder.enc[1] > encoder_target){
          encoder_target = encoder_target - 500;
          last_transition = 112;
          state = 2;
        }
        break;

      case 113: //Go to spinny thing
        mixer.setEdgeMode(true, 0);
        
        if (encoder.enc[1] > encoder_target){
          
          mixer.setVelocity(0.05);
          state = 20;
        }
        
        break;

      case 20: // This is used to approach the spinning thing and stop within 20cm of it
        
        if (dist.dist[0] < 0.25){ //VAR I 0.20
          mixer.setVelocity(0);
          state = 21;
        }
        break;

      case 21: //this is used to speed past the spinning thing when the coast is clear
        if (dist.dist[0] > 0.5){

          mixer.setVelocity(0.6);
          sleep(2);
          encoder_target = encoder.enc[1] + 4900;
          state = 22;
        }

        break;
        

      case 22: //This state speeds up until end of the race track (not in race though just using ti as reference)
        
        mixer.setVelocity(0.2);
        mixer.setEdgeMode(false, 0);
        if (not box){
          sleep(2);
          state = 80;
        } else{
           if (encoder.enc[1] > encoder_target){
          
          // encoder_target = encoder.enc[1] + 11500;
          
            mixer.setVelocity(0.4);
            state = 23;
          }
        }
       
        break;


      case 23:

        if (not medge.edgeValid){
          mixer.setVelocity(0.1);
          sleep(1);
          state = 24;
        }
        break;

      case 24:
        
        if (medge.edgeValid){
          mixer.setEdgeMode(false, 0);
          sleep(1);
          state = 125;
        }
        break;

      case 125:

        if (dist.dist[0] < 0.6){ //0.6

          mixer.setVelocity(0);
          state = 126;
        }

        
        
        break;
      
      case 126: //
        pose.resetPose();
        sleep(2);
        mixer.setDesiredHeading(3.14*0.55);
        sleep(2);
        
        mixer.setVelocity(0);
        sleep(2);
        if (last_transition == 71){
          state = 72;
        } else if (last_transition == 112 or last_transition == 60)
          state = 127;
        break;


      case 127: //Here we go from end position to connection line to circle and more
        mixer.setVelocity(0.2);
        
        if (medge.edgeValid){
          mixer.setEdgeMode(false, 0);
          encoder_target = encoder.enc[1] + 500; //1750
          state = 25; 

        }
        break;
      // case 23: // This sets a new target to get onto new line
      //   if (encoder.enc[1] > encoder_target){
      //     encoder_target = encoder.enc[1] + 3000;
      //     state = 24;
      //   }

      //   break;

      // case 24: //This slows the robot down and goes over first line and starts following next
      //   mixer.setTurnrate(0);
      //   mixer.setVelocity(0.2);
      //   if (encoder.enc[1] > encoder_target && medge.edgeValid){ // BREYTA 3000 i breytu
      //     mixer.setEdgeMode(false, 0);
      //     encoder_target = encoder.enc[1] + 2000;
      //     state = 25;

      //   }

        

      case 25: //This switches to left following after the line was reached
        if (encoder.enc[1] > encoder_target){
          mixer.setEdgeMode(true, 0);
          usleep(50000);
          
          state = 260;
        }
        break;

      case 260:
        if (imu.gyro[2] < -50){
          encoder_target = encoder.enc[1] + 2600;
          state = 26;
        }
        break;

      case 26: // This goes to position to look for moving robot and turns to view it.
        if (encoder.enc[1] > encoder_target){
        
          pose.resetPose();
          mixer.setVelocity(0);
          sleep(2);
          mixer.setDesiredHeading(-3.14*0.3);
          state = 27;
        }
      break;
      
      case 27: // when the track robot moves past go to state 28
        if (dist.dist[0] < 0.25){ //VAR I 0.20
          if (last_transition == 112){
            state = 50;
          } else if (last_transition == 60){
            state = 28;
          }
        }
      break;

      case 50: // Move straight and follow line
        if (dist.dist[0] > 0.5){
          sleep(1);
          mixer.setDesiredHeading(0);
          sleep(1);
          mixer.setEdgeMode(true, 0);
          mixer.setVelocity(0.15);
          sleep(2);
          mixer.setEdgeMode(false, 0);
          state = 51;
        }
      break;

      case 51:
        
        if (imu.gyro[2] > 120){
          usleep(90000);
          mixer.setEdgeMode(true, 0);
          sleep(2);
          mixer.setVelocity(0.3);
          encoder_target = encoder.enc[1] + 3000;
          state = 39;
        }
        break;

      case 28: // Move straight and follow line
        if (dist.dist[0] > 0.5){
          sleep(1);
          mixer.setDesiredHeading(0);
          sleep(1);
          mixer.setEdgeMode(true, 0);
          state = 29;
        }
      break;

      case 29: //Follow line until wall is reached
        mixer.setVelocity(0.2); //SETJA ÞETTA INN AFTUR
        if (dist.dist[0] < 0.15){
          mixer.setVelocity(0.0);
          pose.resetPose();
          sleep(2);
          mixer.setDesiredHeading(3.14*0.015);
          sleep(2);
          pose.resetPose();
          encoder_target = encoder.enc[1] - 4150;

          state = 30;
          
        }
        break;

      case 30: //Back up onto circle thing and go through one gate
        mixer.setVelocity(-0.5);
        if (encoder.enc[1] < encoder_target){
          mixer.setVelocity(0);
          sleep(2);
          
          state = 31;
        }
        break;

      case 31: // Adjust to go through second gate
        
        
        mixer.setVelocity(0);
        sleep(1);
        mixer.setDesiredHeading(3.14*0.45);
        sleep(2);
        encoder_target = encoder.enc[1] + 1500; 
        state = 32;
        

        break;

      case 32: //MOVE THROUGH second gate and part one of get ready for third gate
        pose.resetPose();
        mixer.setVelocity(0.2);
        if (encoder.enc[1] > encoder_target){
          encoder_target = encoder.enc[1] + 500;
          mixer.setVelocity(0);
          pose.resetPose();
          sleep(2);
          mixer.setDesiredHeading(-3.14*0.35);
          
          sleep(2);
          pose.resetPose();
          state = 33;
        }
        break;

      case 33: //After this state we are ready to go through last gate
        mixer.setVelocity(0.2);
        if (encoder.enc[1] > encoder_target){
          mixer.setVelocity(0);
          sleep(2);
          mixer.setDesiredHeading(-3.14*0.35);
          sleep(2);
          pose.resetPose();
          encoder_target = encoder.enc[1] + 1000;
          state = 34;
        }
        break;
      
      case 34:  //Move through third gate
        
        mixer.setVelocity(0.2);
        if (encoder.enc[1] > encoder_target){
          mixer.setVelocity(0);
          sleep(2);
          mixer.setDesiredHeading(-3.14*0.25);
          sleep(2);
          pose.resetPose();
          encoder_target = encoder.enc[1] + 1250;
          state = 35;
        }
        break;
        
      case 35: //Move to end of circle thing
        mixer.setVelocity(0.2);
        if (encoder.enc[1] > encoder_target){
          mixer.setVelocity(0);
          state = 36;
        }

        break;      
      
      case 36: //Wait for circle robot to pass
        if (dist.dist[0] < 0.25){ //VAR I 0.20
          
          state = 37;
        }
        break;

      case 37: //Move onto line after robot passes

        if (dist.dist[0] > 0.35){
          sleep(2);
          mixer.setVelocity(0.2);
          state = 38;
        }
        break;
      
      case 38: //find line and start following

        if (medge.edgeValid){
          mixer.setEdgeMode(false,0);
          encoder_target = encoder.enc[1] + 700;
          state = 39;
        }
      break;


      case 39: //Stop following line and get ready for going to race
        
        if (encoder.enc[1] > encoder_target) {
          mixer.setTurnrate(0);
          pose.resetPose();
          encoder_target = encoder.enc[1] + 3000;
          state = 40;
        }
        break;

      case 40: //EXIT ROUNDABOUT AND FIND NEW LINE (RACE)
        mixer.setVelocity(0.15); 
        if (encoder.enc[1] > encoder_target && medge.edgeValid){ //SETJA ENCODER_TARGET HER I STAÐINN FYRIR 0
          encoder_target = encoder.enc[1] + 1550;
          mixer.setEdgeMode(false,0);
          state = 45;
        }

        break;

      case 45: //GO TO START POSITION ON RACE
        if (encoder.enc[1] > encoder_target){
          mixer.setVelocity(0);
          pose.resetPose();
          sleep(1);
          state = 46;
        }
        break;
      
      case 46: //Positioning for race

        mixer.setDesiredHeading(-3.14*0.25);
        sleep(1);
        mixer.setVelocity(-0.2);
        encoder_target = encoder.enc[1] - 950;
        state = 47;
        break;

      case 47: //Positioning for race
        
        if (encoder.enc[1] < encoder_target){
          pose.resetPose();
          mixer.setVelocity(0);
          sleep(1);
          mixer.setDesiredHeading(3.14*0.1150);
          state = 48;
        }

        break;

      case 48: //Positiong for race
        sleep(2);
        mixer.setVelocity(0.1);
        sleep(2);
        mixer.setVelocity(0);

        sleep(2);
        // sleep(1);
        // mixer.setVelocity(0);
        if (last_transition == 112){
          state = 41;
        } else if (last_transition == 60){
          state = 70;
          
        }
        
        break;


      case 70: //Go from race position to reach line on the other end of spinny thing
      
        mixer.setVelocity(0.3);
        encoder_target = encoder.enc[1] + 3000;
        state = 71;
        break;

      case 71: //Find line and go slowly to get onto the line
        
        if (medge.edgeValid && encoder.enc[1] > encoder_target){
          mixer.setVelocity(0.15);
          mixer.setEdgeMode(true, 0);
          encoder_target = encoder.enc[1] + 4900;
          sleep(1);
          last_transition = 71; 
          state = 22;
        }
        break;
      
      case 72: 
        mixer.setVelocity(0.1);
        
        if (medge.edgeValid){
          mixer.setEdgeMode(true, 0);
          encoder_target = encoder.enc[1] + 2000;
          state = 73;

        }

        break;
      
      case 73:
        if (encoder.enc[1] > encoder_target){

          mixer.setEdgeMode(false,0);
          encoder_target = encoder.enc[1] + 3000;
          state = 74;
        }
        break;

      case 74:
        if (encoder.enc[1] > encoder_target){
          mixer.setEdgeMode(true, 0);
          
        }
        
        break;

        


      case 41: //RACE 41 to 44 DO NOT RUN UNLESS IN PERFECT POSITION IT GOES VEEEEERY FAST
        // mixer.setVelocity(0.6);
        // mixer.setEdgeMode(false, 0);
        // if (encoder.enc[1] > 1500){
          mixer.setVelocity(0);
          // mixer.setTurnrate(0);
          encoder_target = encoder.enc[1] + 12500;
          sleep(2);
          // mixer.setVelocity(0.5);
          // sleep(1);
          mixer.setVelocity(1);
          // // mixer.setTurnrate(0);
          usleep(700000);
          mixer.setVelocity(2);
          state = 42;

        //   mixer.setVelocity(1.6);
        //   sleep(3);
        // }
        break;

      case 42: //This slows the robot down after fast part to catch the race line
        if (encoder.enc[1] > encoder_target){
          mixer.setVelocity(0.3);
          
          state = 43;
        }
        break;

      case 43: //This finds the line in race after slowing down 
        if (medge.edgeValid){
          mixer.setEdgeMode(false, 0);
          encoder_target = encoder.enc[1] + 300;
          state = 44;
        }
            
        
        break;

      case 44: //This case finishes the race
        if (encoder.enc[1] > encoder_target){
          mixer.setVelocity(0.6);
          state = 60;
        }
        break;


      case 60: //This ends the race gracefully
        if (not medge.edgeValid){
          mixer.setTurnrate(0);
          mixer.setVelocity(0.15);
          last_transition = 50;
          encoder_target = encoder.enc[1] + 100;
          last_transition = 60;
          state = 23;
        }

        break;

      
       // case 10: //EKKI I NOTKUN
        
      //   pose.resetPose();
      //   toLog("forward at 0.3m/s");
      //   //mixer.setTurnrate(0.4);
      //   mixer.setVelocity(0.4);
      //   mixer.setEdgeMode(false, 0);
        
        
      //   //sleep(0.5);
      //   state = 11;
        
      //   // finished = true;
      //   break;
      
      // case 11: // wait for distance
      // case 11: //EKKI I NOTKUN
      //   encoder_target = 12000;
      //   if (encoder.enc[1] > encoder_target){
      //     mixer.setVelocity(0.2);
      //     sleep(1);
      //     mixer.setEdgeMode(true, 0);
      //     sleep(2);
      //     mixer.setVelocity(0.4);
      //     state = 12;
      //   }
      //   break;
      // case 12: //EKKI I NOTKUN
      //   encoder_target = 27000;
      //   if (encoder.enc[1] > encoder_target){
      //   mixer.setVelocity(0.2);
      //   sleep(1);
      //   mixer.setEdgeMode(false, 0);
      //   sleep(2);
      //   mixer.setVelocity(0.4);
      //   state = 13;
      // }
      // break;

      // case 13: //EKKI I NOTKUN
      //   encoder_target = 33000;
      //   if (encoder.enc[1] > encoder_target){
      //     mixer.setVelocity(0.2);
      //     sleep(1);
      //     mixer.setEdgeMode(true, 0);
      //     sleep(2);
      //     mixer.setVelocity(0.3);
      //     state = 114;
      // }
      // break;    

      // case 114: //EKKI I NOTKUN
        
        
      //   if (dist.dist[0] < 0.3){
      //     sleep(2);
      //     state = 115;
      //   }
      //   break;

      // case 115: //EKKI I NOTKUN
      //   if (dist.dist[0] < 0.8){
          
      //     state = 14;
      //   }
      //   break;

      // case 14: //EKKI I NOTKUN
       
          
        
      //   mixer.setVelocity(0);
          
      //   state = 15;
        
      //   break;
      
      // case 15: //EKKI I NOTKUN
      //   pose.resetPose();
      //   sleep(2);
      //   mixer.setDesiredHeading(3.14*0.55);
      //   sleep(2);
        
      //   mixer.setVelocity(0);
      //   sleep(2);
        
      //   state = 16;
      //   break;

      // case 16: //Here we go from end position to connection line to circle and more
      //   mixer.setVelocity(0.2);
      //   if (medge.edgeValid){
      //     mixer.setEdgeMode(false, 0);
      //     encoder_target = encoder.enc[1] + 1750;
      //     state = 19; //State 19 to go to turning obstacle

      //   }

      //   break;

      // case 17:
        
      //   //Here we switch to left to go to circle NOT IN USE
      //   if (encoder.enc[1] > encoder_target){
            
      //       mixer.setEdgeMode(true, 0);
      //       state = 18;
      //   }
      //   break;

      // case 18:
      //   //This is used to set the speed slower right before harsh right turn 
      //   if (encoder.enc[1] > encoder_target){
      //       encoder_target = encoder.enc[1] + 700;
      //       state = 19;
      //   }
      //   break;

      // case 19: // this makes the turn to the spinning thing. 

      //   if (encoder.enc[1] > encoder_target) {

      //     mixer.setVelocity(0.05);
      //     encoder_target = encoder.enc[1] + 700;
      //     state = 20;
      //   }
      //   break;




      default:
        toLog("Unknown state");
        lost = true;
        finished = true;
        break;
    }
    // toLog("Didn't enter while loop");
    if (state != oldstate)
    {
      oldstate = state;
      toLog("state start");
      // reset time in new state
      t.now();
    }
    // wait a bit to offload CPU
    usleep(2000);
  
  if (lost)
  { // there may be better options, but for now - stop
    toLog("Plan20 got lost");
    mixer.setVelocity(0);
    mixer.setTurnrate(0);
  }
  // else{toLog("Plan20 finished");}

  }
  
  // teensy1.send("enc0\n");
  // sleep(1);
  // while (not service.stop && encoder.enc[1] < 425){
  //     // pose.resetPose();
  //     // mixer.setVelocity(0.4);
  //     mixer.setTurnrate(2);
  //     finished = true;
  //   }

  // mixer.setVelocity(0);
  // mixer.setTurnrate(0);
  // service.theEnd = true;
}

//Receives the distance in meters and converts it to encoder ticks
int BPlan20::getTicks(float distance){ 
  int ret;
  ret = (int)((distance*1000)/distance_per_tick);
  return ret; 
}


void BPlan20::terminate(){ 
  if (logfile != nullptr){
    fclose(logfile);
    logfile = nullptr;
  }
    
}

void BPlan20::toLog(const char* message)
{
  UTime t("now");
  if (logfile != nullptr)
  {
    fprintf(logfile, "%lu.%04ld %d %% %s\n", t.getSec(), t.getMicrosec()/100,
            oldstate,
            message);
  }
  if (toConsole)
  {
    printf("%lu.%04ld %d %% %s\n", t.getSec(), t.getMicrosec()/100,
           oldstate,
           message);
  }
}




void BPlan20::send_command(const std::string& host, int port, const std::string& command) {
    std::cout << "--------------------------------------------" << std::endl;
    std::cout << "Connecting to TCP server: " << host << ":" << port << std::endl;

    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[1024] = {0};

    // Creating the socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cout << "Socket creation error" << std::endl;
        return;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if(inet_pton(AF_INET, host.c_str(), &serv_addr.sin_addr) <= 0) {
        std::cout << "Invalid address / Address not supported" << std::endl;
        return;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cout << "Connection Failed. Is the server running on " << host << ":" << port << "?" << std::endl;
        return;
    }

    // Send the command
    std::cout << "Sending command: " << command << std::endl;
    send(sock, command.c_str(), command.length(), 0);
    send(sock, "\n", 1, 0); // Ensure to send the newline character

    // Now attempt to read the response
    memset(buffer, 0, sizeof(buffer)); // Clear the buffer
    if(read(sock, buffer, 1024) < 0) {
        std::cout << "Failed to read response" << std::endl;
    } else {
        std::cout << "Received response: " << buffer << std::endl;
    }

    std::string input(buffer);
    std::istringstream iss(input);
    std::string token;
    std::vector<float> numbers;

    // Split the string by comma and convert to float
    while (std::getline(iss, token, ',')) {
        std::istringstream tokenStream(token);
        float number;
        if (tokenStream >> number) {
            numbers.push_back(number);
        }
    }

    dist_to_ball = numbers[0];
    angle_to_ball = numbers[1];
    close(sock);
    
}

