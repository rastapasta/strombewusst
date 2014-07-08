###************************************
*  StromBewusst - Einspeisungsfetcher
# *************************************
# Configuration
###

utils = require "utility"
request = require "request"
csv = require "csv"
xml2js = require "xml2js"
class Import

class ImportTransnet extends Import
  url: (month, type) ->
    if type is "wind"
      "http://www.transnetbw.de/en/key-figures/renewable-energies/wind-infeed?app=wind&activeTab=csv&selectMonat=0&view=1&download=true"
    else
      "http://www.transnetbw.de/en/key-figures/renewable-energies/photovoltaic?app=solar&activeTab=csv&selectMonat=0&view=1&download=true"

class ImportAmprion extends Import
  url: (day, type) ->
    elements = day.split "-"
    tag = "#{elements[2]}.#{elements[1]}.#{elements[0]}"
    console.log tag
    if type is "wind"
      "http://amprion.de/applications/applicationfiles/winddaten2.php?mode=show&day=#{tag}"
    else
      "http://amprion.de/applications/applicationfiles/PV_einspeisung.php?mode=show&day=#{tag}"

  fetch: (day, type, cb) ->
    request @url(day, type),
      (error, response, body) =>
        # Stupid XML malformation on their side
        body = body.replace /"time="/g, '" time="'
        parser = new xml2js.Parser()
        parser.parseString body, (err, result) ->
          console.dir result.amprion.item

class ImportTennet extends Import
  url: (month, type) ->
    if type is "wind"
      "http://www.tennettso.de/site/Transparenz/veroeffentlichungen/netzkennzahlen/tatsaechliche-und-prognostizierte-windenergieeinspeisung/tatsaechliche-und-prognostizierte-windenergieeinspeisung/monthDataSheetCsv?monat=#{month}"
    else
      "http://www.tennettso.de/site/Transparenz/veroeffentlichungen/netzkennzahlen/tatsaechliche-und-prognostizierte-solarenergieeinspeisung_land/monthDataSheetCsv?monat=#{month}"

  fetch: (month, type, cb) ->
    request @url(month, type),
      (error, response, body) =>
        csv.parse body,
          delimiter: ";",
          (err, data) =>
            @parse data, cb

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
          actual: Number row[2]
          forecast: Number row[3]

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

importer = new ImportAmprion
importer.fetch "2014-07-01", "wind" 
importer = new ImportTennet
importer.fetch "2014-07", "wind", (days) -> console.log days
#importer.fetch "2014-07-01", "wind", "actual", (days) -> console.log days
