## Description

This script is designed to interface with the plugin to expose an [OpenTherm](https://en.wikipedia.org/wiki/OpenTherm)-enabled boiler, giving you similar functionality to more expensive smart-home thermostats like [Nest](https://nest.com/thermostats/).

An [OpenTherm adapter](http://ihormelnyk.com/arduino_opentherm_controller) is required to connect the NodeMCU to the boiler. A PCB design is included in the `OpenTherm PCB` folder.

Unlike temperature control via hysteresis, the OpenTherm protocol allows for smoother and more refined heating control via a PID controller:

![image](https://www.sunnyheat-crea-systems.com/images/about_sunnyheat/sunnyheat_reg_en.png)

## Requirements

* NodeMCU

* [OpenTherm](https://en.wikipedia.org/wiki/OpenTherm)-compatible appliance

* [OpenTherm adapter](http://ihormelnyk.com/arduino_opentherm_controller) (PCB design included in the `OpenTherm PCB` folder)

* DHT11 Temperature Sensor

* Micro-USB cable

## How-to

1. First, install the `ArduinoJson`, `PID`, `OpenTherm Library`, `DHT`, and [`PID AutoTune`](https://github.com/br3ttb/Arduino-PID-AutoTune-Library) libraries from the _Library manager_ in the Arduino IDE, then follow [this](https://gist.github.com/phenotypic/8d9d3b886936ccea9c21f495755640dd) gist which walks you through how to flash a NodeMCU. The `.ino` file referred to in the gist is the `NodeMCU-OpenTherm.ino` file included in this repository - only seems to work reliably with `esp8266` library `<=2.5.1`

2. Build [this circuit](http://ihormelnyk.com/arduino_opentherm_controller) in order to be able to interface the NodeMCU with the appliance

3. Assuming that you already have [homebridge](https://github.com/homebridge/homebridge#installation) set up, the next thing you will have to do is install the plugin:
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
       "currentRelativeHumidity": true,
       "dhw": true
    }
]
```

## Wiring

**Note:** `OT Input` and `OT Output` refer to the input and output of the OpenTherm adapter circuit, _not_ direct connections to the boiler

| NodeMCU | OpenTherm Adapter | DHT11  |
| --- | --- | --- |
| `D1` | `OT Input` | |
| `D2` | `OT Output` | |
| `3V3` | | `VCC (+)` |
| `GND` | | `GND (-)` |
| `D6` | | `OUT` |

## Tuning

The `Kp`, `Ki`, and `Kd` values included the script may require tuning in order to better fit your home's heating needs. The script includes an auto-tuner for this purpose.

To use the tuner, first start the heating as normal then wait for the ambient temperature and the boiler temperature to stabilise (could take a few hours), then request `/tuningState/1` from the NodeMCU via `curl`.

This will initiate the tuning which may take a number of hours. Read [this](http://brettbeauregard.com/blog/2012/01/arduino-pid-autotune-library/) blog for details on how the tuner works, but bear in mind that while tuning is taking place, your home may become uncomfortably warm/cool. This is normal and is necessary for the tuner.

Once tuning is complete, you should note down the recommended values as they will not be saved on the NodeMCU. These values are ballpark figures, giving you a better idea, but you may want to further tune your system manually.

## Notes

- You can request `/diag` (diagnostics) from the NodeMCU via `curl` in order to receive more detailed information
