# homebridge-web-boiler

[![npm](https://img.shields.io/npm/dt/homebridge-web-boiler.svg)](https://www.npmjs.com/package/homebridge-web-boiler) [![npm](https://img.shields.io/npm/v/homebridge-web-boiler.svg)](https://www.npmjs.com/package/homebridge-web-boiler)

## Description

This [homebridge](https://github.com/nfarina/homebridge) plugin exposes a web-based boiler to Apple's [HomeKit](http://www.apple.com/ios/home/). Using simple HTTP requests, the plugin allows you to turn on/off the boiler as well as control the target temperature.

## Installation

1. Install [homebridge](https://github.com/nfarina/homebridge#installation-details)
2. Install this plugin: `npm install -g homebridge-web-boiler`
3. Update your `config.json` file

## Configuration

```json
"accessories": [
     {
       "accessory": "Boiler",
       "name": "Boiler",
       "apiroute": "http://myurl.com"
     }
]
```

### Core
| Key | Description | Default |
| --- | --- | --- |
| `accessory` | Must be `Boiler` | N/A |
| `name` | Name to appear in the Home app | N/A |
| `apiroute` | Root URL of your device | N/A |
| `pollInterval` _(optional)_ | Time (in seconds) between device polls | `60` |

### Optional fields
| Key | Description | Default |
| --- | --- | --- |
| `temperatureDisplayUnits` _(optional)_ | Whether you want °C (`0`) or °F (`1`) as your units | `0` |
| `currentRelativeHumidity` _(optional)_ | Whether to include `currentRelativeHumidity` as a field in `/status` | `false` |
| `maxTemp` _(optional)_ | Upper bound for the temperature selector in the Home app | `30` |
| `minTemp` _(optional)_ | Lower bound for the temperature selector in the Home app | `15` |

### Additional options
| Key | Description | Default |
| --- | --- | --- |
| `timeout` _(optional)_ | Time (in milliseconds) until the accessory will be marked as _Not Responding_ if it is unreachable | `3000` |
| `http_method` _(optional)_ | HTTP method used to communicate with the device | `GET` |
| `username` _(optional)_ | Username if HTTP authentication is enabled | N/A |
| `password` _(optional)_ | Password if HTTP authentication is enabled | N/A |
| `model` _(optional)_ | Appears under the _Model_ field for the accessory | `homebridge-web-boiler` |
| `serial` _(optional)_ | Appears under the _Serial_ field for the accessory | apiroute |
| `manufacturer` _(optional)_ | Appears under the _Manufacturer_ field for the accessory | `Tom Rodrigues` |

## API Interfacing

Your API should be able to:

1. Return JSON information when it receives `/status`:
```
{
    "targetHeatingCoolingState": INT_VALUE,
    "targetTemperature": INT_VALUE,
    "currentHeatingCoolingState": INT_VALUE,
    "currentTemperature": FLOAT_VALUE
}
```

**Note:** You must also include `currentRelativeHumidity` in `/status` if enabled in the `config.json`

2. Set `targetHeatingCoolingState` when it receives:
```
/targetHeatingCoolingState/INT_VALUE
```

3. Set `targetTemperature` when it receives:
```
/targetTemperature/INT_VALUE
```

## HeatingCoolingState Key

| Number | Name |
| --- | --- |
| `0` | Off |
| `1` | Heat |
