var Service, Characteristic
const packageJson = require('./package.json')
const request = require('request')
const ip = require('ip')
const http = require('http')

module.exports = function (homebridge) {
  Service = homebridge.hap.Service
  Characteristic = homebridge.hap.Characteristic
  homebridge.registerAccessory('homebridge-web-boiler', 'Boiler', Boiler)
}

function Boiler (log, config) {
  this.log = log

  this.name = config.name
  this.apiroute = config.apiroute
  this.pollInterval = config.pollInterval || 300

  this.listener = config.listener || false
  this.port = config.port || 2000
  this.requestArray = ['targetHeatingCoolingState', 'targetTemperature', 'dhwTargetState', 'dhwTargetTemperature']

  this.manufacturer = config.manufacturer || packageJson.author.name
  this.serial = config.serial || this.apiroute
  this.model = config.model || packageJson.name
  this.firmware = config.firmware || packageJson.version

  this.username = config.username || null
  this.password = config.password || null
  this.timeout = config.timeout || 3000
  this.http_method = config.http_method || 'GET'

  this.temperatureDisplayUnits = config.temperatureDisplayUnits || 0

  this.currentRelativeHumidity = config.currentRelativeHumidity || false
  this.chMin = config.chMin || 15
  this.chMax = config.chMax || 30

  this.dhw = config.dhw || false
  this.dhwName = config.dhwName || 'Hot Water'
  this.dhwMin = config.dhwMin || 40
  this.dhwMax = config.dhwMax || 50

  if (this.username != null && this.password != null) {
    this.auth = {
      user: this.username,
      pass: this.password
    }
  }

  if (this.listener) {
    this.server = http.createServer(function (request, response) {
      var parts = request.url.split('/')
      var partOne = parts[parts.length - 2]
      var partTwo = parts[parts.length - 1]
      if (parts.length === 3 && this.requestArray.includes(partOne)) {
        this.log('Handling request: %s', request.url)
        response.end('Handling request')
        this._httpHandler(partOne, partTwo)
      } else {
        this.log.warn('Invalid request: %s', request.url)
        response.end('Invalid request')
      }
    }.bind(this))

    this.server.listen(this.port, function () {
      this.log('Listen server: http://%s:%s', ip.address(), this.port)
    }.bind(this))
  }
}

Boiler.prototype = {

  identify: function (callback) {
    this.log('Identify requested!')
    callback()
  },

  _httpRequest: function (url, body, method, callback) {
    request({
      url: url,
      body: body,
      method: this.http_method,
      timeout: this.timeout,
      rejectUnauthorized: false,
      auth: this.auth
    },
    function (error, response, body) {
      callback(error, response, body)
    })
  },

  _getStatus: function (callback) {
    var url = this.apiroute + '/status'
    this.log.debug('Getting status: %s', url)

    this._httpRequest(url, '', this.http_method, function (error, response, responseBody) {
      if (error) {
        this.log.warn('Error getting status: %s', error.message)
        this.chService.getCharacteristic(Characteristic.CurrentHeatingCoolingState).updateValue(new Error('Polling failed'))
        if (this.dhw) {
          this.dhwService.getCharacteristic(Characteristic.CurrentHeatingCoolingState).updateValue(new Error('Polling failed'))
        }
        callback(error)
      } else {
        this.log.debug('Device response: %s', responseBody)
        var json = JSON.parse(responseBody)
        this.chService.getCharacteristic(Characteristic.TargetTemperature).updateValue(json.targetTemperature)
        this.log('CH | Updated TargetTemperature to: %s', json.targetTemperature)
        this.chService.getCharacteristic(Characteristic.CurrentTemperature).updateValue(json.currentTemperature)
        this.log('CH | Updated CurrentTemperature to: %s', json.currentTemperature)
        this.chService.getCharacteristic(Characteristic.TargetHeatingCoolingState).updateValue(json.targetHeatingCoolingState)
        this.log('CH | Updated TargetHeatingCoolingState to: %s', json.targetHeatingCoolingState)
        this.chService.getCharacteristic(Characteristic.CurrentHeatingCoolingState).updateValue(json.currentHeatingCoolingState)
        this.log('CH | Updated CurrentHeatingCoolingState to: %s', json.currentHeatingCoolingState)
        if (this.currentRelativeHumidity) {
          this.chService.getCharacteristic(Characteristic.CurrentRelativeHumidity).updateValue(json.currentRelativeHumidity)
          this.log('CH | Updated CurrentRelativeHumidity to: %s', json.currentRelativeHumidity)
        }
        if (this.dhw) {
          this.dhwService.getCharacteristic(Characteristic.TargetTemperature).updateValue(json.dhwTargetTemperature)
          this.log('DHW | Updated TargetTemperature to: %s', json.dhwTargetTemperature)
          this.dhwService.getCharacteristic(Characteristic.CurrentTemperature).updateValue(json.dhwCurrentTemperature)
          this.log('DHW | Updated CurrentTemperature to: %s', json.dhwCurrentTemperature)
          this.dhwService.getCharacteristic(Characteristic.TargetHeatingCoolingState).updateValue(json.dhwTargetState)
          this.log('DHW | Updated TargetHeatingCoolingState to: %s', json.dhwTargetState)
          this.dhwService.getCharacteristic(Characteristic.CurrentHeatingCoolingState).updateValue(json.dhwCurrentState)
          this.log('DHW | Updated CurrentHeatingCoolingState to: %s', json.dhwCurrentState)
        }
        callback()
      }
    }.bind(this))
  },

  _httpHandler: function (characteristic, value) {
    switch (characteristic) {
      case 'targetHeatingCoolingState':
        this.chService.getCharacteristic(Characteristic.TargetHeatingCoolingState).updateValue(value)
        this.log('CH | CH | Updated %s to: %s', characteristic, value)
        break
      case 'targetTemperature':
        this.chService.getCharacteristic(Characteristic.TargetTemperature).updateValue(value)
        this.log('CH | Updated %s to: %s', characteristic, value)
        break
      case 'dhwTargetState':
        this.dhwService.getCharacteristic(Characteristic.TargetHeatingCoolingState).updateValue(value)
        this.log('DHW | Updated %s to: %s', characteristic, value)
        break
      case 'dhwTargetTemperature':
        this.dhwService.getCharacteristic(Characteristic.TargetTemperature).updateValue(value)
        this.log('DHW | Updated %s to: %s', characteristic, value)
        break
      default:
        this.log.warn('Unknown characteristic "%s" with value "%s"', characteristic, value)
    }
  },

  setTargetHeatingCoolingState: function (value, callback) {
    var url = this.apiroute + '/targetHeatingCoolingState/' + value
    this.log.debug('CH | Setting targetHeatingCoolingState: %s', url)

    this._httpRequest(url, '', this.http_method, function (error, response, responseBody) {
      if (error) {
        this.log.warn('CH | Error setting targetHeatingCoolingState: %s', error.message)
        callback(error)
      } else {
        this.log('CH | Set targetHeatingCoolingState to: %s', value)
        callback()
      }
    }.bind(this))
  },

  setTargetTemperature: function (value, callback) {
    var url = this.apiroute + '/targetTemperature/' + value
    this.log.debug('CH | Setting targetTemperature: %s', url)

    this._httpRequest(url, '', this.http_method, function (error, response, responseBody) {
      if (error) {
        this.log.warn('CH | Error setting targetTemperature: %s', error.message)
        callback(error)
      } else {
        this.log('CH | Set targetTemperature to: %s', value)
        callback()
      }
    }.bind(this))
  },

  setDHWState: function (value, callback) {
    var url = this.apiroute + '/dhwTargetState/' + value
    this.log.debug('DHW | Setting targetHeatingCoolingState: %s', url)

    this._httpRequest(url, '', this.http_method, function (error, response, responseBody) {
      if (error) {
        this.log.warn('DHW | Error setting targetHeatingCoolingState: %s', error.message)
        callback(error)
      } else {
        this.log('DHW | Set targetHeatingCoolingState to: %s', value)
        callback()
      }
    }.bind(this))
  },

  setDHWTemperature: function (value, callback) {
    var url = this.apiroute + '/dhwTargetTemperature/' + value
    this.log.debug('DHW | Setting targetTemperature: %s', url)

    this._httpRequest(url, '', this.http_method, function (error, response, responseBody) {
      if (error) {
        this.log.warn('DHW | Error setting targetTemperature: %s', error.message)
        callback(error)
      } else {
        this.log('DHW | Set targetTemperature to: %s', value)
        callback()
      }
    }.bind(this))
  },

  getServices: function () {
    this.informationService = new Service.AccessoryInformation()
    this.informationService
      .setCharacteristic(Characteristic.Manufacturer, this.manufacturer)
      .setCharacteristic(Characteristic.Model, this.model)
      .setCharacteristic(Characteristic.SerialNumber, this.serial)
      .setCharacteristic(Characteristic.FirmwareRevision, this.firmware)

    this.chService = new Service.Thermostat(this.name, 1)
    this.chService.getCharacteristic(Characteristic.TemperatureDisplayUnits).updateValue(this.temperatureDisplayUnits)

    this.chService
      .getCharacteristic(Characteristic.TargetHeatingCoolingState)
      .on('set', this.setTargetHeatingCoolingState.bind(this))

    this.chService
      .getCharacteristic(Characteristic.TargetTemperature)
      .on('set', this.setTargetTemperature.bind(this))

    this.chService.getCharacteristic(Characteristic.TargetHeatingCoolingState)
      .setProps({
        maxValue: Characteristic.TargetHeatingCoolingState.HEAT
      })

    this.chService.getCharacteristic(Characteristic.CurrentTemperature)
      .setProps({
        minValue: -100,
        maxValue: 100,
        minStep: 0.1
      })

    this.chService.getCharacteristic(Characteristic.TargetTemperature)
      .setProps({
        minValue: this.chMin,
        maxValue: this.chMax,
        minStep: 1
      })

    var services = [this.informationService, this.chService]

    if (this.dhw) {
      this.dhwService = new Service.Thermostat(this.dhwName, 2)
      this.dhwService.getCharacteristic(Characteristic.TemperatureDisplayUnits).updateValue(this.temperatureDisplayUnits)

      this.dhwService
        .getCharacteristic(Characteristic.TargetHeatingCoolingState)
        .on('set', this.setDHWState.bind(this))

      this.dhwService
        .getCharacteristic(Characteristic.TargetTemperature)
        .on('set', this.setDHWTemperature.bind(this))

      this.dhwService.getCharacteristic(Characteristic.TargetHeatingCoolingState)
        .setProps({
          maxValue: Characteristic.TargetHeatingCoolingState.HEAT
        })

      this.dhwService.getCharacteristic(Characteristic.CurrentTemperature)
        .setProps({
          minValue: -100,
          maxValue: 100,
          minStep: 0.1
        })

      this.dhwService.getCharacteristic(Characteristic.TargetTemperature)
        .setProps({
          minValue: this.dhwMin,
          maxValue: this.dhwMax,
          minStep: 0.5
        })
      services.push(this.dhwService)
    }

    this._getStatus(function () {})

    setInterval(function () {
      this._getStatus(function () {})
    }.bind(this), this.pollInterval * 1000)

    return services
  }
}
