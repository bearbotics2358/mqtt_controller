# mqtt_controller

This program resides on the Raspberry Pi and permits switching between Computer Vision and Remote Viewing via MQTT commands.

To control the cameras, send one of three messages:
- off
- view
- vision

to one of two cameras
- claw
- cargo

Examples command-line commands:
$ mosquitto_pub -t "/camera/controls/claw/" -m "vision"
$ mosquitto_pub -t "/camera/controls/cargo/" -m "view"
