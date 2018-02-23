# Habitat Hub
Habitat Hub: An Alexa and Arduino Smart Home for terrariums.

Need help keeping your pet healthy and safe, use this environmental monitoring unit that attaches to your reptile or amphibians terrarium.

### About
My son got a ball python for Christmas.  I objected to him getting one but you guest it.  I lost that battle.  So I joined him on this journey.  My major concern was him taking care of his new friend because of the care that is needed for this type of snake.  Temperatures at different spots in the terrarium, humidity and keeping the water bowl full.  That's a big list for a teenager.  This is where the idea for this project came about.

Habitat Hub is an Arduino based unit with multiple sensors and display that helps monitor environmental conditions of a reptile or amphibian terrarium.  The display is used to rotates through the temperature areas, humidity, water level and last time the water bowl and cage was cleaned.  If there is WiFi access, the unit has the ability to connect to Amazon Alexa to report back these conditions.

For reptiles and amphibians it's important to regulate their environment to mimic their natural habitat and to keep a clean surroundings.  This project will monitor the following: 

1. Ambient Temperature
2. Cool Spot
3. Hot Spot
4. Bowl water level
5. Date of last cage cleaning
6. Date of last water cleaning

### What's Here
1. **AlexaHabHubCustomSkill** - this is the AWS Lambda function for the Habitat Hub Alexa Custom Skill.
2. **AlexaHabHubInteractionModel** - this is a json file with the InterationModel that will be copied into the Alexa developers interface.
3. **Arduino ARV** - This code is for the Arduino Yun AVR side that monitors the sensors and sends the data to the middle ware application on the Linux side.
4. **Arduino Linux** - This is the middle ware that stores configuration data from the ARV, receives sensor data and pushes it to the IoT Shadow.
5. **README.md** - You're reading it now.