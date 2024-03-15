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
  state = 27;
  int encoder_target = 0;//BREYTA I 0
  oldstate = state;
  //
  // toLog("Plan20 started");
  //
  
  
  while (not finished and not lost and not service.stop)
  {
    
    switch (state)
    { // make a shift in heading-mission
      case 9: // TESTING GO DOWN DO NOT USE
        mixer.setVelocity(0.2);
        mixer.setEdgeMode(true, 0);
        break;


      case 10:
        
        pose.resetPose();
        toLog("forward at 0.3m/s");
        //mixer.setTurnrate(0.4);
        mixer.setVelocity(0.4);
        mixer.setEdgeMode(false, 0);
        
        
        //sleep(0.5);
        state = 11;
        
        // finished = true;
        break;
      
      // case 11: // wait for distance
      case 11:
      encoder_target = 12000;
      if (encoder.enc[1] > encoder_target){
        mixer.setVelocity(0.2);
        sleep(1);
        mixer.setEdgeMode(true, 0);
        sleep(2);
        mixer.setVelocity(0.4);
        state = 12;
      }
      break;
      case 12:
        encoder_target = 27000;
        if (encoder.enc[1] > encoder_target){
        mixer.setVelocity(0.2);
        sleep(1);
        mixer.setEdgeMode(false, 0);
        sleep(2);
        mixer.setVelocity(0.4);
        state = 13;
      }
      break;

      case 13:
        encoder_target = 32000;
        if (encoder.enc[1] > encoder_target){
          mixer.setVelocity(0.2);
          sleep(1);
          mixer.setEdgeMode(true, 0);
          sleep(2);
          mixer.setVelocity(0.2);
          state = 14;
      }
      break;


      case 14:
       
          
        if (dist.dist[0] < 0.7 && encoder.enc[1] > 40000 && not service.stop){ //&& encoder.enc[1] > 41100
          mixer.setVelocity(0);
          
          state = 15;
        }
        break;
      
      case 15:
        pose.resetPose();
        sleep(2);
        mixer.setDesiredHeading(3.14*0.55);
        sleep(2);
        
        mixer.setVelocity(0);
        sleep(2);
        
        state = 16;
        break;

      case 16: //Here we go from end position to connection line to circle and more
        mixer.setVelocity(0.2);
        if (medge.edgeValid){
          mixer.setEdgeMode(false, 0);
          encoder_target = encoder.enc[1] + 1750;
          state = 19; //State 19 to go to turning obstacle

        }

        break;

      case 17:
        
        //Here we switch to left to go to circle NOT IN USE
        if (encoder.enc[1] > encoder_target){
            mixer.setEdgeMode(true, 0);
            state = 18;
        }
        break;

      case 18:
        //This is used to set the speed slower right before harsh right turn 
        if (encoder.enc[1] > encoder_target){
            state = 19;
        }
        break;

      case 19: // this makes the turn to the spinning thing. 

        if (encoder.enc[1] > encoder_target) {

          mixer.setVelocity(0.05);
          encoder_target = encoder.enc[1] + 700;
          state = 20;
        }
        break;

      case 20: // This is used to approach the spinning thing and stop within 20cm of it
        if (dist.dist[0] < 0.25 && (encoder.enc[1] > encoder_target)){ //VAR I 0.20
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
        
        if (encoder.enc[1] > encoder_target){
          mixer.setVelocity(0.4);
          encoder_target = encoder.enc[1] + 11500;
          
          state = 23;
        }

        break;

      case 23: // This sets a new target to get onto new line
        if (encoder.enc[1] > encoder_target){
          encoder_target = encoder.enc[1] + 3000;
          state = 24;
        }

        break;

      case 24: //This slows the robot down and goes over first line and starts following next
        mixer.setTurnrate(0);
        mixer.setVelocity(0.2);
        if (encoder.enc[1] > encoder_target && medge.edgeValid){ // BREYTA 3000 i breytu
          mixer.setEdgeMode(false, 0);
          encoder_target = encoder.enc[1] + 2000;
          state = 25;

        }

        break;

      case 25: //This switches to left following after the line waas reached
        if (encoder.enc[1] > encoder_target){
          mixer.setEdgeMode(true, 0);
          encoder_target = encoder.enc[1] + 2700;
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
          state = 28;

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
        mixer.setVelocity(-0.3);
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
      
      case 38:

        if (medge.edgeValid){
          mixer.setEdgeMode(false,0);
          encoder_target = encoder.enc[1] + 700;
          state = 39;
        }
      break;


      case 39:
        
        if (encoder.enc[1] > encoder_target) {
          mixer.setTurnrate(0);
          pose.resetPose();
          encoder_target = encoder.enc[1] + 3000;
          state = 40;
        }
        break;

      case 40: //EXIT ROUNDABOUT AND FIND NEW LINE (RACE)
        mixer.setVelocity(0.2); //DELETE
        if (encoder.enc[1] > encoder_target && medge.edgeValid){ //SETJA ENCODER_TARGET HER I STAÐINN FYRIR 0
          encoder_target = encoder.enc[1] + 1750;
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
      
      case 46:

        mixer.setDesiredHeading(-3.14*0.25);
        sleep(1);
        mixer.setVelocity(-0.2);
        encoder_target = encoder.enc[1] - 950;
        state = 47;
        break;

      case 47:
        
        if (encoder.enc[1] < encoder_target){
          pose.resetPose();
          mixer.setVelocity(0);
          sleep(1);
          mixer.setDesiredHeading(3.14*0.1150);
          state = 48;
        }

        break;

      case 48:
        sleep(2);
        mixer.setVelocity(0.1);
        sleep(2);
        mixer.setVelocity(0);

        sleep(2);
        // sleep(1);
        // mixer.setVelocity(0);
        state = 41;
        break;

      case 41: //RACE 41 to 44 DO NOT RUN UNLESS IN PERFECT POSITION IT GOES VEEEEERY FAST
        // mixer.setVelocity(0.6);
        // mixer.setEdgeMode(false, 0);
        // if (encoder.enc[1] > 1500){
          mixer.setVelocity(0);
          // mixer.setTurnrate(0);
          encoder_target = encoder.enc[1] + 12250;
          sleep(2);
          // mixer.setVelocity(0.5);
          // sleep(1);
          mixer.setVelocity(1);
          // // mixer.setTurnrate(0);
          sleep(1);
          mixer.setVelocity(2);
          state = 42;

        //   mixer.setVelocity(1.6);
        //   sleep(3);
        // }
        break;

      case 42:
        if (encoder.enc[1] > encoder_target){
          mixer.setVelocity(0.3);
          
          state = 43;
        }
        break;

      case 43:
        if (medge.edgeValid){
          mixer.setEdgeMode(false, 0);
          encoder_target = encoder.enc[1] + 300;
          state = 44;
        }
            
        
        break;

      case 44:
        if (encoder.enc[1] > encoder_target){
          mixer.setVelocity(0.7);
        }
        break;


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

