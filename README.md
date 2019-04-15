# homebridge-web-boiler

#### Homebridge plugin to control a web-based boiler

## Description

homebridge-web-boiler exposes a boiler to HomeKit and makes it controllable via HTTP requests. The plugin will poll your boiler at regular intervals and present you with this information when requested. The plugin also allows you so control a number boiler variables via HomeKit such as the target temperature.

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
| `apiroute` | Root URL of your Boiler device (excluding the rest of the requests) | N/A |
| `pollInterval` _(optional)_ | Time (in seconds) between when homebridge will check the `/status` of your boiler | `60` |

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
| `timeout` _(optional)_ | Time (in milliseconds) until the accessory will be marked as "Not Responding" if it is unreachable | `3000` |
| `http_method` _(optional)_ | The HTTP method used to communicate with the boiler | `GET` |
| `username` _(optional)_ | Username if HTTP authentication is enabled | N/A |
| `password` _(optional)_ | Password if HTTP authentication is enabled | N/A |
| `model` _(optional)_ | Appears under the "Model" field for the device | `homebridge-web-boiler` |
| `serial` _(optional)_ | Appears under the "Serial" field for the device | apiroute |
| `manufacturer` _(optional)_ | Appears under the "Manufacturer" field for the device | `Tom Rodrigues` |

## API Interfacing

Your API should be able to:

1. Return boiler info when it receives `/status` in the JSON format like below:
```
{
    "targetHeatingCoolingState": INT_VALUE_0_TO_1,
    "targetTemperature": FLOAT_VALUE,
    "currentHeatingCoolingState": INT_VALUE_0_TO_1,
    "currentTemperature": FLOAT_VALUE
}
```

**Note:** You must also include `currentRelativeHumidity` in `/status` if enabled in the `config.json`

2. Set `targetHeatingCoolingState` when it receives:
```
/targetHeatingCoolingState/{INT_VALUE_0_TO_1}
```

3. Set `targetTemperature` when it receives:
```
/targetTemperature/{INT_VALUE}
```

## HeatingCoolingState Key

| Number | Name |
| --- | --- |
| `0` | Off |
| `1` | Heat |
