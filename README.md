# Kratos - Fuchsia
CSSE4011: Advanced Embedded Systems 2025
Project by Alice Barnett, Aidian Liaw, & Mac Rogers

Topic E
Accelerometer-based terrain mapping using an RC car and iBeacon localisation.

A mobile node will be attached to an RC car which will be driven through a field of iBeacon transmitters at known locations. An on-board accelerometer will read changes in z axis acceleration and use this to interpret changes in height across the terrain. (small ramps will be placed by the team to simulate this). The mobile node then relays this information back to the base node over a BLE communication network and subsequently updates a 2D map of terrain height on a tago.io web dashboard. The RC car is also equipped with an ultrasonic sensor which will play a tone when the sensor detects an obstacle within 100mm of the front of the car, preventing reckless driving.

Deliverables:
- initiation of alert tone when ultrasonic detects obstacle within 100mm
- frequent and accurate polling of the Thingy52's IMU accelerometer z-axis
- BLE communication between mobile and base nodes
- calculation of height differences based on acceleration changes
- visualisation of data within the web dashboard
