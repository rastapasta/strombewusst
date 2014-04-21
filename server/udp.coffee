###***************************
*  StromBewusst - UDP Server
# ****************************
# Configuration
###
SERVER_PORT = 8888
#***

db = require("mongojs").connect "strombewusst", ["meters", "signals"]
dgram = require "dgram"
server = dgram.createSocket "udp4"

findByKey = (key) ->
  db.meters.find key: key  

server.on "message", (msg, rinfo) ->
  # Decode message -> 6byte API key - 1byte framesize - 1byte impulses
  data = new String msg
  
  if data.length != 8
    console.log "[data] invalid data received from #{rinfo.address}: #{data}"
    return

  key = ''
  for i in [0..5]
    key += data.charCodeAt(i).toString 16

  timeframe = data.charCodeAt 6
  impulses = data.charCodeAt 7

  console.log "[data] #{rinfo.address}: #{key} -> #{impulses} impulses in #{timeframe} seconds"
  meter = db.meters.findOne key: key, (err, meter) ->
    unless meter
      console.log "[data] couldn't find corresponding meter"
    else
      db.signals.save meter: meter._id, timeframe: timeframe, impulses: impulses, (err) ->
        if err
          console.log "[data] wasn't able to save... #{err}"
        else
          console.log "[data] saved!"

server.on "listening", () ->
  address = server.address()
  console.log "*** StromBewusst - UDP Server ***"
  console.log "[server] listening on #{address.address}:#{address.port}"

server.bind SERVER_PORT


