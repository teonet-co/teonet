/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 2; tab-width: 4 -*- */
/*
 * teoasync_c application
 *
 * main.cpp
 * Copyright (C) Kirill Scherba 2018 <kirill@scherba.ru>
 *
 * teoasync_c is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * teoasync_c is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * File:   main.cpp
 * Author: Kirill Scherba <kirill@scherba.ru>
 *
 * teoasync_c c++ application.
 * 
 * Application API:
 * 
 *  \TODO Describe server API here
 *  CMD_USER: ...
 *      @input ...
 *      @output ...
 * 
 * Created on December 20, 2017, 9:52 AM
 */

#include <atomic>
#include <chrono>
#include <thread>

using namespace std::chrono_literals;

#include "teonet.hpp"

// Add configuration header
#undef PACKAGE
#undef VERSION
#undef GETTEXT_PACKAGE
#undef PACKAGE_VERSION
#undef PACKAGE_TARNAME
#undef PACKAGE_STRING
#undef PACKAGE_NAME
#undef PACKAGE_BUGREPORT
#include "config.h"
#include "utils.h"

#define APP_VERSION VERSION

class MyTeonet : public teo::Teonet {

// Constructor & destructor
public:

  MyTeonet(int argc, char** argv) : teo::Teonet(argc, argv) {
    std::cout << "Constructor myTeonet\n";
  }

  virtual ~MyTeonet() {
    std::cout << "Destructor myTeonet\n";
  }
  
// Threads methods and data  
private:

  static const int NUM_PROCESS = 10;
  static const int NUM_RECORDS = 10000;
  std::thread first[NUM_PROCESS];
  
  // Process
  void process(int id) {
    int j, i = 0;
    showMessage("Process " + std::to_string(id) + " started\n");
    while (getKe()->runEventMgr) {
        double t = getTime();
        std::string loop = "process " + std::to_string(id) + " loop " + std::to_string(++i);
        showMessage(loop + " started\n");
        for(j = 1; j <= NUM_RECORDS; j++) {
            std::string msg = "Hello " + std::to_string(id) + " " + std::to_string(i) + " " + std::to_string(j) + "!";
            sendToA("teo-async-s" /*getHostName()*/, 129, msg);
            showMessage("send: " + msg + "\n");
            if(!getKe()->runEventMgr) goto exit;
        }
        showMessage(loop + " send " + std::to_string(j-1) + " records during " + std::to_string(getTime()-t) + " sec\n");
        std::this_thread::sleep_for(500ms);
    }
    
exit:

    showMessage("Process " + std::to_string(id) + " stopped\n");
  }

  /**
   * Start threads
   */
  void start_thread(int id = 1) {
    first[id-1] = std::thread([this, id] { process(id); });
  }

  /**
   * Stop thread
   */
  void stop_thread(int id = 1) {
    first[id-1].join();
  }  

// Own class methods and data
public:

//  void showMessage(const std::string msg) {
//    std::cout << msg;
//  }

  /**
   * Teonet event handler
   *
   * @param event Teonet event
   * @param data Pointer to data
   * @param data_length Data length
   * @param user_data Pointer to user data
   */
  void eventCb(teo::teoEvents event, void *data, size_t data_length,
    void *user_data) override {

    switch (event) {

      // Calls when event started  
      case EV_K_STARTED: {
        showMessage("Event: EV_K_STARTED, Teonet class version: " +
          (std::string)getClassVersion() + "\n");
        for(int id=1; id <= NUM_PROCESS; id++) start_thread(id);
      } break;

      // Calls when peer connected to this application
      case EV_K_CONNECTED: {
        // Teonet packet
        auto rd = getPacket(data);
        showMessage("Event: EV_K_CONNECTED, "
          "from: " + (const std::string)rd->from + ",\t"
          "data: " + (const char*) rd->data + "\n");

        // Request host info
        sendTo(rd->from, CMD_HOST_INFO, NULL, 0); //(void*)"JSON", 5);
      } break;

      // Calls when date received from a peer
      case EV_K_RECEIVED: {

        // Teonet packet
        auto rd = getPacket(data);

        switch (rd->cmd) {

          case CMD_ECHO_ANSWER:
            break;

          case CMD_HOST_INFO_ANSWER: {

            auto host_info = teo::HostInfo(rd->data);
            showMessage("Event: EV_K_RECEIVED, "
              "cmd: CMD_HOST_INFO_ANSWER, from: " + (std::string)rd->from + ", "
              "name: " + host_info.name + ", " +
              "version: " + host_info.version + ", " +
              "teonet version: " + host_info.teonetVersion + ", " +
              "types: " + host_info.type.to_string() +
              "\n");

          } break;

          default: {
            showMessage("Event: EV_K_RECEIVED, "
              "cmd: " + std::to_string(rd->cmd) + ", "
              "from: " + (const char*) rd->from + ",\t"
              "data: " + (const char*) rd->data + "\n");
          } break;
        }

      } break;
      
      // Call before event manager stopped
      case EV_K_STOPPED_BEFORE:
        for(int id=1; id <= NUM_PROCESS; id++) stop_thread(id);
        break;

      // Calls after event manager stopped
      case EV_K_STOPPED:
        showMessage("Event: EV_K_STOPPED\n");
        break;

      default:
        break;

    }
  }

};
 
/**
 * Main teoasync_c application function
 *
 * @param argc Number of parameters
 * @param argv Parameters array
 *
 * @return EXIT_SUCCESS
 */
int main(int argc, char** argv) {

  std::cout << "Teonet async test C++ application ver " APP_VERSION ", based on teonet ver "
    << teoGetLibteonetVersion() << "\n";

  // Initialize teonet event manager and Read configuration
  auto teo = new MyTeonet(argc, argv);

  // Set application type
  teo->setAppType("teoasync_c");
  teo->setAppVersion(APP_VERSION);

  // Start teonet
  teo->run();

  delete(teo);

  return (EXIT_SUCCESS);
}
