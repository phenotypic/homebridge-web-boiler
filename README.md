# homebridge-web-boiler

[![npm](https://img.shields.io/npm/v/homebridge-web-boiler.svg)](https://www.npmjs.com/package/homebridge-web-boiler) [![npm](https://img.shields.io/npm/dt/homebridge-web-boiler.svg)](https://www.npmjs.com/package/homebridge-web-boiler)

## Description

This [homebridge](https://github.com/nfarina/homebridge) plugin exposes a web-based boiler to Apple's [HomeKit](http://www.apple.com/ios/home/). Using simple HTTP requests, the plugin allows you to control your central heating as well as your hot water if you enable it in the `config.json`.

Find script samples for the boiler controller in the _examples_ folder.

## Installation

1. Install [homebridge](https://github.com/nfarina/homebridge#installation-details)
2. Install this plugin: `npm install -g homebridge-web-boiler`
3. Update your `config.json` file

## Configuration

```json
"accessories": [
     {
       "accessory": "Boiler",
       "name": "Thermostat",
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

### Optional fields
| Key | Description | Default |
| --- | --- | --- |
| `temperatureDisplayUnits` | Whether you want °C (`0`) or °F (`1`) as your units | `0` |
| `currentRelativeHumidity` | Whether to include `currentRelativeHumidity` as a field in `/status` | `false` |
| `chMin` | Lower bound for the temperature selector in the Home app | `15` |
| `chMax` | Upper bound for the temperature selector in the Home app | `30` |
| `dhw` | Whether you want to expose hot water control as an extra accessory | `false` |
| `dhwName` | Name for the extra hot water accessory | `Hot Water` |
| `dhwMin` | Lower bound for the hot water in the Home app | `40` |
| `dhwMax` | Upper bound for the hot water in the Home app | `50` |

### Additional options
| Key | Description | Default |
| --- | --- | --- |
| `pollInterval` | Time (in seconds) between device polls | `300` |
| `listener` | Whether to start a listener to get real-time changes from the device | `false` |
| `timeout` | Time (in milliseconds) until the accessory will be marked as _Not Responding_ if it is unreachable | `3000` |
| `port` | Port for your HTTP listener (if enabled) | `2000` |
| `http_method` | HTTP method used to communicate with the device | `GET` |
| `username` | Username if HTTP authentication is enabled | N/A |
| `password` | Password if HTTP authentication is enabled | N/A |
| `model` | Appears under the _Model_ field for the accessory | plugin |
| `serial` | Appears under the _Serial_ field for the accessory | apiroute |
| `manufacturer` | Appears under the _Manufacturer_ field for the accessory | author |
| `firmware` | Appears under the _Firmware_ field for the accessory | version |

## API Interfacing

Your API should be able to:

1. Return JSON information when it receives `/status`:
```
{
    "targetHeatingCoolingState": INT_VALUE,
    "targetTemperature": FLOAT_VALUE,
    "currentHeatingCoolingState": INT_VALUE,
    "currentTemperature": FLOAT_VALUE
}
```

**Note:** You must also include the following fields in `/status` where relevant:

- `currentRelativeHumidity` (if `currentRelativeHumidity` is enabled)
- `dhwTargetState` (if `dhw` is enabled)
- `dhwCurrentState` (if `dhw` is enabled)
- `dhwTargetTemperature` (if `dhw` is enabled)
- `dhwCurrentTemperature` (if `dhw` is enabled)

2. Set `targetHeatingCoolingState` when it receives:
```
/targetHeatingCoolingState/INT_VALUE
```

3. Set `targetTemperature` when it receives:
```
/targetTemperature/FLOAT_VALUE
```

4. Set `dhwTargetState` when it receives: (if `dhw` is enabled)
```
/dhwTargetState/INT_VALUE
```

5. Set `dhwTargetTemperature` when it receives: (if `dhw` is enabled)
```
/dhwTargetTemperature/FLOAT_VALUE
```

### Optional (if listener is enabled)

1. Update `targetHeatingCoolingState` following a manual override by messaging the listen server:
```
/targetHeatingCoolingState/INT_VALUE
```

2. Update `targetTemperature` following a manual override by messaging the listen server:
```
/targetTemperature/FLOAT_VALUE
```

3. Update `dhwTargetState` following a manual override by messaging the listen server: (if `dhw` is enabled)
```
/dhwTargetState/INT_VALUE
```

4. Update `dhwTargetTemperature` following a manual override by messaging the listen server: (if `dhw` is enabled)
```
/dhwTargetTemperature/FLOAT_VALUE
```
