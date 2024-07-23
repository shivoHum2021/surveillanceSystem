# Surveillance System

## Table of Contents
- [Overview](#Overview)
- [Requirements](#requirements)
- [Installation](#installation)
- [Usage](#usage)
- [Options](#options)
- [Components](#components)
- [License](#license)

## Overview

The Surveillance System is a comprehensive application designed for real-time video processing, object detection, event-driven analytics, and cloud-based storage. 
It operates with three main processes:

**xVision**: Responsible for frame capture and initial processing.

**CVRDaemon**: Manages continuous video recording.

**SurveillanceSystem**: Handles event detection, analytics, and interaction with cloud services.

**Features**

-Frame Capture: Captures frames from cameras using CaptureFrameFromCamera.

-Object Detection: Utilizes TensorFlow Lite for object detection and processes frames with high efficiency.

-Event-Driven Analytics: Analyzes frames for motion events and other significant activities.

-Video Recording: Records video continuously and saves it locally or uploads it to the cloud.

-Cloud Integration: Uploads significant events and video segments to the cloud for storage and further analysis.

-Efficient Memory Management: Handles frame buffers and TensorFlow Lite interpreter invocation efficiently to minimize memory usage.

## Requirements
- CMake 3.5 or higher
- A C++17 compatible compiler
- log4cplus
- curl
- OpenCV (core, imgproc, imgcodecs)
- TensorFlow Lite (if `USE_TENSOR_LITE` is enabled)
- TVM (if `USE_TVM` is enabled)
- PlumerAI (if `USE_PLUMERAI_MODEL` is enabled)


## Installation

1. Clone the repository:

```sh
Clone the repository:
git clone https://github.com/shivoHum2021/surveillanceSystem.git
```
2.  Create a build directory and navigate into it:
```sh
mkdir build
cd build
```
3. Configure the project with CMake:
```sh 
cmake ..
```
4. Build the project:
```sh
make
```
## Usage
After building the project, you can run the surveillanceApp executable. The application supports various options and configurations based on the CMake options enabled during the build process.
```sh
./surveillanceApp
```

## Options
The project supports several options that can be enabled or disabled at build time:

- USE_TVM: Compile with TVM support (default: OFF)
- USE_TENSOR_LITE: Compile with TensorLite support (default: OFF)
- USE_PLUMERAI_MODEL: Compile with PlumerAI support (default: OFF)
- ENABLE_CLASSIFICATION: Compile with object classification support (default: OFF)

These options can be set when running the CMake configuration step:

```sh
cmake -DUSE_TENSOR_LITE=ON -DENABLE_CLASSIFICATION=ON ..
```
## Components
**Logger**

The logger component is responsible for logging throughout the application. It uses the log4cplus library.

**HTTP Client**

The HTTP client component handles HTTP requests and responses. It uses the curl library.

**Frame Processing**

The frame processing component handles operations related to camera frame handling and processing. It uses OpenCV for image processing tasks.

**Model Processing**

The model processing component provides support for different machine learning frameworks. Based on the options enabled, it can support TensorFlow Lite, TVM, and PlumerAI models.

**Main Application**

The main application (surveillanceApp) integrates all the components and manages the overall surveillance system operations.

## License

This project is licensed under the MIT License. See the LICENSE file for details.



