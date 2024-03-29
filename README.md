<p align="center">
  <a href="https://github.com/homebridge/homebridge"><img src="https://raw.githubusercontent.com/homebridge/branding/master/logos/homebridge-color-round-stylized.png" height="140"></a>
</p>

<span align="center">

# homebridge-web-boiler

[![npm](https://img.shields.io/npm/v/homebridge-web-boiler.svg)](https://www.npmjs.com/package/homebridge-web-boiler) [![npm](https://img.shields.io/npm/dt/homebridge-web-boiler.svg)](https://www.npmjs.com/package/homebridge-web-boiler)

</span>

## Description

This [homebridge](https://github.com/homebridge/homebridge) plugin exposes a web-based boiler to Apple's [HomeKit](http://www.apple.com/ios/home/). Using simple HTTP requests, the plugin allows you to control your central heating as well as your hot water (if enabled in the `config.json`).

See the `examples` folder for ready-to-use NodeMCU boiler control scripts, featuring both hysteresis and OpenTherm implementations. Hysteresis maintains the temperature within a set range, while OpenTherm controls temperature more precicely by modulating the boiler's output.

## Installation

1. Install [homebridge](https://github.com/homebridge/homebridge#installation)
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
| `minStep` | Minimum increment value for the temperature selector in the Home app | `0.5` |
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
| `checkupDelay` | Time (in milliseconds) after setting `HeatingCoolingState` to update `targetHeatingCoolingState` and `currentHeatingCoolingState` | `2000` |
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
/targetHeatingCoolingState?value=INT_VALUE
```

3. Set `targetTemperature` when it receives:
```
/targetTemperature?value=FLOAT_VALUE
```

4. Set `dhwTargetState` when it receives: (if `dhw` is enabled)
```
/dhwTargetState?value=INT_VALUE
```

5. Set `dhwTargetTemperature` when it receives: (if `dhw` is enabled)
```
/dhwTargetTemperature?value=FLOAT_VALUE
```

### Optional (if listener is enabled)

1. Update `targetHeatingCoolingState` following a manual override by messaging the listen server:
```
/targetHeatingCoolingState?value=INT_VALUE
```

2. Update `targetTemperature` following a manual override by messaging the listen server:
```
/targetTemperature?value=FLOAT_VALUE
```

3. Update `dhwTargetState` following a manual override by messaging the listen server: (if `dhw` is enabled)
```
/dhwTargetState?value=INT_VALUE
```

4. Update `dhwTargetTemperature` following a manual override by messaging the listen server: (if `dhw` is enabled)
```
/dhwTargetTemperature?value=FLOAT_VALUE
```
