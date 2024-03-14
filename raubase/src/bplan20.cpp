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
  state = 10;
  oldstate = state;
  //
  // toLog("Plan20 started");
  //
  
  
  while (not finished and not lost and not service.stop)
  {
    if (dist.dist[0] < 0.3 && encoder.enc[1] > 41100){ //&& encoder.enc[1] > 41100
      finished = true;
    }
    switch (state)
    { // make a shift in heading-mission
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
      case 11: // wait for distance
        if (pose.dist >= 30) //in x direction
        { // done, and then
          finished = true;
          service.stop = true;
        }
        else if (t.getTimePassed() > 120){
          lost = true;
          service.stop = true;
          
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
  teensy1.send("enc0\n");
  sleep(1);
  while (not service.stop && encoder.enc[1] < 425){
      // pose.resetPose();
      // mixer.setVelocity(0.4);
      mixer.setTurnrate(2);
      finished = true;
    }

  mixer.setVelocity(0);
  mixer.setTurnrate(0);
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
