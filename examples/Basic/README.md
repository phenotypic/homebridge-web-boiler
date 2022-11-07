## Description

This script is designed to interface with the plugin to expose a boiler. The script controls the boiler using a relay. See below:

![image](https://guide.openenergymonitor.org/images/integrations/relay-hist.png)

## Requirements

* NodeMCU

* Relay

* DHT11 Temperature Sensor

* Micro-USB cable

## How-to

1. First, install the `ArduinoJson` and `DHT` libraries from the _Library manager_ in the Arduino IDE, then follow [this](https://gist.github.com/phenotypic/8d9d3b886936ccea9c21f495755640dd) gist which walks you through how to flash a NodeMCU. The `.ino` file referred to in the gist is the `NodeMCU-Basic.ino` file included in this repository

2. Assuming that you already have [homebridge](https://github.com/homebridge/homebridge#installation) set up, the next thing you will have to do is install the plugin:
```
npm install -g homebridge-web-boiler
```

3. Finally, update your `config.json` file following the example below:

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

| NodeMCU | Relay Module | DHT11  |
| --- | --- | --- |
| `3V3` | `VCC` | |
| `GND` | `GND` | |
| `D7` | `IN1` | |
| `3V3` | | `VCC (+)` |
| `GND` | | `GND (-)` |
| `D6` | | `OUT` |
