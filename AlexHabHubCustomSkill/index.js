console.log('Loading...');
const Alexa = require('alexa-sdk');
require('datejs');
// Replace the next variables with your settings
// You can find this on the Amazon Alexa Console for you
// custom skill.
const APP_ID = 'amzn1.ask.skill.673df69f-c1ce-4dbd-83a4-94c6b4ee4334';

var config = {};
// Replace the next three variables with your settings
// You can find this info on the Iot Console for your thing.
config.IOT_SENSOR_BROKER_ENDPOINT = "a2y4w0zykqn9g5.iot.us-east-1.amazonaws.com";
config.IOT_SENSOR_BROKER_REGION = "us-east-1";
config.IOT_SENSOR_BROKER_THING_NAME = "habhub_1234";


const languageStrings = {
    'en': {
        translation: {
            SKILL_NAME: 'Habitat Hub',
            WELCOME_MESSAGE: "Welcome to %s. You can ask a question like, what\'s the temperature of hot spot? ... Now, what can I help you with?",
            WELCOME_REPROMPT: 'For instructions on what you can say, please say help me.',
            DISPLAY_CARD_TITLE: '',
            HELP_MESSAGE: "You can ask questions such as, what\'s the temperature of cool spot, or, you can say exit...Now, what can I help you with?",
            HELP_REPROMPT: "You can say things like, what\'s the temperature of ambient, or you can say exit...Now, what can I help you with?",
            STOP_MESSAGE: 'Goodbye!'
        }
    },
    'en-US': {
        translation: {
            SKILL_NAME: 'Habitat Hub',
            TEMP_UNIT: 'fahrenheit'
        }
    }
};

String.prototype.splice = function(index, count, add) {
    if (index < 0) {
      index = this.length + index;
      if (index < 0) {
        index = 0;
      }
    }
    return this.slice(0, index) + (add || "") + this.slice(index + count);
};

function formatDate(date) {
    var monthNames = [
      "January", "February", "March",
      "April", "May", "June", "July",
      "August", "September", "October",
      "November", "December"
    ];
  
    var day = date.getDate();
    var monthIndex = date.getMonth();
    var year = date.getFullYear();
  
    return day + ' ' + monthNames[monthIndex] + ' ' + year;
};

function getlastCleanDate(lsdNum) {
    var lsdStr = lsdNum.toString();
    lsdStr = lsdStr.splice(4,0,"-").splice(7,0,"-");
    console.log(lsdStr);
    var lsdDte = Date.parse(lsdStr);
    return formatDate(lsdDte);
}

function getThingShadow(callback) {
    var Aws = require('aws-sdk');
    Aws.config.region = config.IOT_SENSOR_BROKER_REGION;

    var iotData = new Aws.IotData({ endpoint: config.IOT_SENSOR_BROKER_ENDPOINT });
    var params = {
        thingName: config.IOT_SENSOR_BROKER_THING_NAME
    };

    iotData.getThingShadow(params, function (err, data) {
        if (err) {
            console.log(err, err.stack);
            callback("failed");
        }
        else {
            console.log("Sensor Thing Shadow = ");
            console.log(data);
            var sensorObject = JSON.parse(data.payload).state.desired;
            console.log(sensorObject);
            callback(sensorObject);
        }
    });
}

function buildTempSentance(tempSensor, tempValue, tempUnit) {
    var response = "";
    console.log(tempSensor);
    console.log(tempValue);
    if (tempValue !== null && tempValue !== -99) {
        response = 'The temperature of ' + tempSensor + ' is ' + tempValue + ' degrees '+ tempUnit + '. ';
    }
    console.log("Return Temp: " + response);
    return response;
}

function buildHumiditySentance(lastSensorData) {
    var value = lastSensorData.humidity;
    console.log(value);
    var response = "";
    if (value !== null && value !== -99) {
        response = "The humidity is " + value + " percent. ";
    }
    console.log("Return Humidity: " + response);
    return response;
}

function buildWaterLevelSentance(lastSensorData) {
    var value = lastSensorData.waterlevel;
    console.log(value);
    var response = "";
    if (value !== null && value !== -99) {
        response = 'The waterlevel is ' + value + '. ';
    }
    console.log("Return Waterlevel: " + response);
    return response;
}

function buildAllTempSentance(lastSensorData, tempUnit) {
    var response = "";
    response += buildTempSentance('hot spot', lastSensorData.hottemp, tempUnit);
    response += buildTempSentance('cool spot', lastSensorData.cooltemp, tempUnit);
    response += buildTempSentance('ambient', lastSensorData.ambtemp, tempUnit);
    console.log("Return AllTemps:" + response);

    return response;
}

function buildAllLastCleanDates(lastSensorData) {
    var response = "";
    try {
        if (lastSensorData.lastcageclean !== null || lastSensorData.lastcageclean !== 0) {
            response += 'The last time the cage was clean was on ' + getlastCleanDate(lastSensorData.lastcageclean);
        }
    }
    catch(err) {
        console.log(err);
        response = "Last cage clean date is not available at this time."
    }
    try {
        if (lastSensorData.lastwaterclean !== null || lastSensorData.lastwaterclean !== 0) {
            response += 'The last time the water was clean was on ' + getlastCleanDate(lastSensorData.lastwaterclean);
        }
    }
    catch(err) {
        console.log(err);
        response = "Last water clean date is not available at this time."
    }
    console.log("Return AllCleanDates: " + response);
    return response;
}

function getStatusText(lastSensorData, tempUnit) {
    var response = 'Your Habitat Hub status is, ';
    response +=  buildAllTempSentance(lastSensorData, tempUnit);
    response +=  buildHumiditySentance(lastSensorData)
    response +=  buildWaterLevelSentance(lastSensorData);    
    response +=  buildAllLastCleanDates(lastSensorData);
    console.log("Status Response: " + response);
    return response;
}

const handlers = {
    'LaunchRequest': function () {
        console.log("Starting LaunchRequest");
        const speechOutput = this.t('WELCOME_MESSAGE', this.t('SKILL_NAME'));
        const reprompt = this.t('WELCOME_REPROMPT');

        this.response.speak(speechOutput).listen(reprompt);
        this.emit(':responseReady');
    },
    'getStatus': function () {
        console.log("Starting getStatus");
        getThingShadow(result => {
            let lastSensorData = result;
            console.log("These are the latest values retrieved from HabHub ");
            console.log(lastSensorData);
            if (lastSensorData === null) {
                this.response.speak('Sensor did not return data.');
                this.emit(':reponseReady');
            } else {
                this.response.speak(getStatusText(lastSensorData, this.t('TEMP_UNIT')));
                this.emit(':responseReady');
            }
        });
    },
    'getLastCleanWaterDate': function () {
        console.log("Starting getLastCleanWaterDate");
        getThingShadow(result => {
            let lastSensorData = result;
            console.log("These are the latest values retrieved from HabHub ");
            console.log(lastSensorData);
            if (lastSensorData === null) {
                this.response.speak('Sensor did not return data.');
                this.emit(':reponseReady');
            } else {
                console.log(lastSensorData.lastwaterclean);
                if (lastSensorData.lastwaterclean === null || lastSensorData.lastwaterclean === 0) {
                    this.response.speak('Sensor did not return data.');
                    this.emit(':responseReady');
                } else {
                    var lsdNum = lastSensorData.lastwaterclean;
                    try {
                        this.response.speak('The last time the water was clean was on ' + getlastCleanDate(lsdNum));
                    }
                    catch(err) {
                        console.log(err);
                        response = "Last water clean date is not available at this time."
                    }
                    this.emit(':responseReady');
                }
            }
        });
    },
    'getLastCleanCageDate': function () {
        console.log("Starting getLastCleanWaterDate");
        getThingShadow(result => {
            let lastSensorData = result;
            console.log("These are the latest values retrieved from HabHub ");
            console.log(lastSensorData);
            if (lastSensorData === null) {
                this.response.speak('Sensor did not return data.');
                this.emit(':reponseReady');
            } else {
                console.log(lastSensorData.lastcageclean);
                if (lastSensorData.lastcageclean === null || lastSensorData.lastcageclean === 0) {
                    this.response.speak('Sensor did not return data.');
                    this.emit(':responseReady');
                } else {
                    var lsdNum = lastSensorData.lastcageclean;
                    try {
                        this.response.speak('The last time the cage was clean was on ' + getlastCleanDate(lsdNum));
                    }
                    catch(err) {
                        console.log(err);
                        response = "Last cage clean date is not available at this time."
                    }
                    this.emit(':responseReady');
                }
            }
        });
    },
    'getHumidity': function () {
        console.log("Starting getHumidity");
        getThingShadow(result => {
            let lastSensorData = result;
            console.log(lastSensorData);
            console.log("These are the latest values retrieved from HabHub " + lastSensorData);
            if (lastSensorData === null) {
                this.response.speak('Sensor did not return data.');
                this.emit(':reponseReady');
            } else {

                if (lastSensorData.humidity === null || lastSensorData.humidity === -99) {
                    this.response.speak('Sensor did not return data.');
                    this.emit(':responseReady');
                } else {
                    this.response.speak('The humidity is ' + lastSensorData.humidity + ' percent');
                    this.emit(':responseReady');
                }
            }
        });
    },
    'getTemperature': function () {
        const tempType = this.event.request.intent.slots.tempType;
        console.log("Starting getTemperature for "+ tempType.value);

        let tempSensor;
        let tempValue;
        if (tempType && tempType.value) {
            tempSensor = tempType.value.toLowerCase();
        }
        //TODO Get Temperature here.

        getThingShadow(result => {
            let lastSensorData = result;
            console.log(lastSensorData);
            console.log("These are the latest values retrieved from HabHub " + lastSensorData);
            if (lastSensorData === null) {
                this.response.speak('Sensor did not return data.');
                this.emit(':reponseReady');
            } else {
                switch (tempSensor) {
                    case 'hot spot':
                        tempValue = lastSensorData.hottemp;
                        break;
                    case 'cool spot':
                        tempValue = lastSensorData.cooltemp;
                        break;
                    case 'ambient':
                        tempValue = lastSensorData.ambtemp;
                        break;
                }

                if (tempValue === null || tempValue === 0) {
                    this.response.speak('Sensor did not return data.');
                    this.emit(':responseReady');
                } else {
                    this.response.speak('The temperature of ' + tempSensor + ' is ' + tempValue + ' degrees '+ this.t('TEMP_UNIT'));
                    this.emit(':responseReady');
                }

            }
        });
    },
    'AMAZON.HelpIntent': function () {
        const speechOutput = this.t('HELP_MESSAGE');
        const reprompt = this.t('HELP_REPROMPT');

        this.response.speak(speechOutput).listen(reprompt);
        this.emit(':responseReady');
    },
    'AMAZON.CancelIntent': function () {
        this.response.speak(this.t('STOP_MESSAGE'));
        this.emit(':responseReady');
    },
    'AMAZON.StopIntent': function () {
        this.response.speak(this.t('STOP_MESSAGE'));
        this.emit(':responseReady');
    },
    'SessionEndedRequest': function () {
        console.log(`Session ended: ${this.event.request.reason}`);
    },
    'Unhandled': function () {
        const speechOutput = this.t('HELP_MESSAGE');
        const reprompt = this.t('HELP_REPROMPT');

        this.response.speak(speechOutput).listen(reprompt);
        this.emit(':responseReady');
    }

};

exports.handler = function (event, context, callback) {
    if (event !== null) {
        console.log('event = ' + JSON.stringify(event));
        const alexa = Alexa.handler(event, context, callback);
        alexa.appId = APP_ID;
        alexa.resources = languageStrings;
        alexa.registerHandlers(handlers);
        alexa.execute();
    }
    else {
        console.log('No event object');

    }
};
