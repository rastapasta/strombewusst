###************************************
*  StromBewusst - Einspeisungsfetcher
# *************************************
# Configuration
###

utils = require "utility"
request = require "request"
csv = require "csv"

class Import
  days: []

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

  fetchPart: (day, type, part) ->
    request @url(day, type, part),
      (error, response, body) =>
        body = body.replace /^jQuery\(/, ""
        body = body.replace /\);$/, ""
        @parse JSON.parse(body)

  fetch: (type, day, cb) ->
    @fetchPart day, "forecast"
    @fetchPart day, "produced"

  parse: (data, cb) ->
    i = 0

    for row in data 
      day = utils.YYYYMMDD new Date row.Date
      time = new Date row.FromTime

      @days[day] = [] unless days[day]
      @days[day][i++] =
        time: time
        estimated: row.Value

importer = new Import50Hertz
importer.fetch "2014-07-01", (days) ->
  console.log days