Surveillance System

Overview

The Surveillance System is a comprehensive application designed for real-time video processing, object detection, event-driven analytics, and cloud-based storage. It operates with three main processes:

xVision: Responsible for frame capture and initial processing.
CVRDaemon: Manages continuous video recording.
SurveillanceSystem: Handles event detection, analytics, and interaction with cloud services.
Features

Frame Capture: Captures frames from cameras using CaptureFrameFromCamera.
Object Detection: Utilizes TensorFlow Lite for object detection and processes frames with high efficiency.
Event-Driven Analytics: Analyzes frames for motion events and other significant activities.
Video Recording: Records video continuously and saves it locally or uploads it to the cloud.
Cloud Integration: Uploads significant events and video segments to the cloud for storage and further analysis.
Efficient Memory Management: Handles frame buffers and TensorFlow Lite interpreter invocation efficiently to minimize memory usage.
Requirements

C++17 or later
TensorFlow Lite
log4cplus
cURL
openCV

Installation
Clone the repository:
git clone https://github.com/shivoHum2021/surveillanceSystem.git


