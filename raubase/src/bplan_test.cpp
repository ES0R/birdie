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
#include <opencv2/calib3d.hpp>
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
#include "maruco.h"
#include "scam.h"
#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "bplan_test.h"

// create class object
BPlan_test plan_test;


void BPlan_test::setup()
{ // ensure there is default values in ini-file
  if (not ini["plan_test"].has("log"))
  { // no data yet, so generate some default values
    ini["plan_test"]["log"] = "true";
    ini["plan_test"]["run"] = "false";
    ini["plan_test"]["print"] = "true";
  }
  // get values from ini-file
  toConsole = ini["plan_test"]["print"] == "true";
  //
  if (ini["plan_test"]["log"] == "true")
  { // open logfile
    std::string fn = service.logPath + "log_plan_test.txt";
    logfile = fopen(fn.c_str(), "w");
    fprintf(logfile, "%% Mission plan_test logfile\n");
    fprintf(logfile, "%% 1 \tTime (sec)\n");
    fprintf(logfile, "%% 2 \tMission state\n");
    fprintf(logfile, "%% 3 \t%% Mission status (mostly for debug)\n");
  }
  setupDone = true;
}

BPlan_test::~BPlan_test()
{
  terminate();
}



void BPlan_test::run()
{
  if (not setupDone)
    setup();
  if (ini["plan_test"]["run"] == "false")
    return;
  //
  UTime t("now");
  bool finished = false;
  bool lost = false;
  state = 10;
  oldstate = state;
  //
  toLog("Plan_test started");
  //
  while (not finished and not lost and not service.stop)
  {
    switch (state)
    { // Test ArUco plan
      case 10:
      { // brackets to allow local variables
        pose.resetPose();
        cv::Mat frame = cam.getFrameRaw();
        cv::imwrite("img_test.jpg", frame);
        state = 11;
      }
      case 11:
        send_command() //LOOK HERE FIX
        state = 99; // Move to a finished or next state after calibration
      case 99:
        finished = true;
        //break;
      default:
        toLog("Unknown state");
        lost = true;
        break;
    }
    if (state != oldstate)
    {
      oldstate = state;
      toLog("state start");
      // reset time in new state
      t.now();
    }
    // wait a bit to offload CPU
    usleep(2000);
  }
  if (lost)
  { // there may be better options, but for now - stop
    toLog("Plan_test got lost");
    mixer.setVelocity(0);
    mixer.setTurnrate(0);
  }
  else
    toLog("Plan_test finished");
}


void BPlan_test::terminate()
{ //
  if (logfile != nullptr)
    fclose(logfile);
  logfile = nullptr;
}

void BPlan_test::toLog(const char* message)
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



void send_command(const std::string& host, int port, const std::string& command) {
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

    // Receive the initial welcome message from the server
    read(sock, buffer, 1024);
    std::cout << "Received initial message: " << buffer << std::endl;

    // Send the command
    std::cout << "Sending command: " << command << std::endl;
    send(sock, command.c_str(), command.length(), 0);
    send(sock, "\r\n", 2, 0); // Make sure to send the newline characters as well

    memset(buffer, 0, sizeof(buffer)); // Clear the buffer before receiving new data
    // Wait for and print the specific response to the command
    read(sock, buffer, 1024);
    std::cout << "Received response: " << buffer << std::endl;

    close(sock);
}


