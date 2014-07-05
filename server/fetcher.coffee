###************************************
*  StromBewusst - Einspeisungsfetcher
# *************************************
# Configuration
###

utils = require "utility"
request = require "request"
csv = require "csv"

class Import


ImportTennet =
  fetch: (month, cb) ->
    request "http://www.tennettso.de/site/Transparenz/veroeffentlichungen/netzkennzahlen/tatsaechliche-und-prognostizierte-windenergieeinspeisung/tatsaechliche-und-prognostizierte-windenergieeinspeisung/monthDataSheetCsv?monat=#{month}",
      (error, response, body) ->
        csv.parse body,
          delimiter: ";",
          (err, data) ->
            ImportTennet.parse data, cb

  parse: (data, cb) ->
    current = today = false
    day = days = []

    for row in data
      if row[0].match /^\d{4}-\d{2}-\d{2}$/
        if current
          days[utils.YYYYMMDD today] = day
          day = []

        current = Date.parse row[0]
        today = new Date current

      if current
        time = new Date (current + (row[1]-1)*15*60*1000)

        day[row[1]-1] =
          time: time
          produced: Number row[2]
          estimated: Number row[3]

    cb days

class Import50Hertz extends Import
  url: (day, type, part) ->
    if type is "wind"
      if part is "forecast"
        "http://ws.50hertz.com/web01/api/WindPowerForecast/ListRecords?filterDateTime=#{day}&callback=jQuery"
      else
        "http://ws.50hertz.com/web01/api/WindPowerActual/ListRecords?filterDateTime=#{day}&callback=jQuery"
    else
      if part is "forecast"
        "http://ws.50hertz.com/web01/api/PhotovoltaicForecast/ListRecords?filterDateTime=#{day}&callback=jQuery"
      else
        "http://ws.50hertz.com/web01/api/PhotovoltaicActual/ListRecords?filterDateTime=#{day}&callback=jQuery"

  fetch: (day, type, part, cb) ->
    @type = type
    @day = day
    @part = part

    request @url(day, type, part),
      (error, response, body) =>
        #console.log "resonse: #{body}"

        @parse JSON.parse(body.replace(/^jQuery\(/, "").replace(/\);$/, "")), cb

  parse: (data, cb) ->
    i = 0
    days = []
    for row in data
      day = utils.YYYYMMDD new Date row.Date
      time = new Date row.FromTime

      entry = time: time
      entry[@part] = row.Value

      days[day] = [] unless days[day]
      days[day][i++] = entry

    cb days

importer = new Import50Hertz
importer.fetch "2014-07-01", "wind", "actual", (days) -> console.log days
