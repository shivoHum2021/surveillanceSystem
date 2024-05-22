
#include "SurveillanceSystem.hpp"
#include "RTMessageBroker.hpp"
#include "Logger.hpp"

#include <log4cplus/configurator.h>
#include <csignal>

volatile std::sig_atomic_t stop;

void signalHandler(int signum)
{
  stop = 1;
}
using namespace ::camera;
using namespace ::camera::camera_ml;

int main(int argc, char *argv[])
{
  std::signal(SIGINT, signalHandler); // Handle Ctrl+C signal
  log4cplus::initialize();
  log4cplus::PropertyConfigurator::doConfigure("/opt/log4cplus.properties");
  std::string eventConfPath = "/opt/usr_config/tn_upload.conf";
#ifdef ENABLE_CLASSIFICATION
  std::string personModelPath = "/etc/mediapipe/models/xcv-person-detection-224x224-440k.tflite";
  std::string deliveryModelPath = "/etc/mediapipe/models/xcv-delivery-detection-224x224-v2.3.2.tflite";
  std::string deviceName = "cpu";
  LOG_INFO("Starting operations in SurveillanceSystem.");
  SurveillanceSystem *survSystem = new SurveillanceSystem(1, personModelPath, deliveryModelPath, eventConfPath, deviceName);
#endif
#ifndef ENABLE_CLASSIFICATION
  SurveillanceSystem *survSystem = new SurveillanceSystem(1, eventConfPath);
#endif
  RTMessageBroker messageBroker(survSystem);
  messageBroker.rtMsgInit();
  survSystem->startSurveillance();
  messageBroker.notify("start");
  LOG_INFO("Main thread is free to perform other tasks. Press Ctrl+C to stop.");

  while (!stop)
  {
    std::this_thread::sleep_for(std::chrono::seconds(1)); // Sleep to reduce CPU usage
  }

  LOG_INFO("Shutting down the surveillance system...");
  // Perform any cleanup here
  messageBroker.notify("stop");
  if (survSystem)
  {

    delete survSystem;
    survSystem = nullptr;
  }
  return 0;
}
