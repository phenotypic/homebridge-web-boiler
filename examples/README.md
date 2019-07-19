## Description

This script is designed to interface with the plugin to expose an [OpenTherm](https://en.wikipedia.org/wiki/OpenTherm)-enabled boiler, giving you similar functionality to more expensive smart-home thermostats like [Nest](https://nest.com/thermostats/).

## Requirements

* NodeMCU

* [OpenTherm](https://en.wikipedia.org/wiki/OpenTherm)-compatible appliance

* [OpenTherm adapter](http://ihormelnyk.com/arduino_opentherm_controller)

* DHT11 Temperature Sensor

* Micro-USB cable

## How-to

1. First, install the `PID` and `OpenTherm Library` libraries from the _Library manager_ in the Arduino IDE, then follow [this](https://gist.github.com/Tommrodrigues/8d9d3b886936ccea9c21f495755640dd) gist which walks you through how to flash a NodeMCU. The `.ino` file referred to in the gist is the `NodeMCU-Boiler.ino` file included in this repository

2. Build [this circuit](http://ihormelnyk.com/arduino_opentherm_controller) (find PCB files in the `PCB` folder in this repository or purchase from [here](https://www.openhardware.io/view/704/OpenTherm-Adapter)) in order to be able to interface the NodeMCU with the appliance

3. Assuming that you already have [homebridge](https://github.com/nfarina/homebridge#installation) set up, the next thing you will have to do is install the plugin:
```
npm install -g homebridge-web-boiler
```

4. Finally, update your `config.json` file following the example below:

```json
"accessories": [
    {
       "accessory": "Boiler",
       "name": "Boiler",
       "apiroute": "http://boiler.local",
       "currentRelativeHumidity": true
    }
]
```

## Wiring

![Diagram](https://i.ibb.co/rpHztcr/Untitled-1.jpg)

**Note:** `OT input` and `OT input` refer to the input and output of the OpenTherm adapter circuit and **not** direct connections to the boiler

## Notes

- You can request `/diag` (diagnostics) from the NodeMCU via `curl` in order to receive info like the current state and the boiler temperature.