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

server.on "listening", () ->
  address = server.address()
  console.log "*** StromBewusst - UDP Server ***"
  console.log "[server] listening on #{address.address}:#{address.port}"

server.on "message", (buffer, rinfo) ->
  # Decode message -> 6byte API key - 2byte current load

  if buffer.length != 26
    console.log "[data] invalid data received from #{rinfo.address}: #{buffer.length} #{buffer}"
    return

  key = ""
  for i in [0..22]
    key += String.fromCharCode(buffer[i]);
    
  load = buffer.readUInt8(24)*256+buffer.readUInt8(25)*1

  console.log "[data] #{rinfo.address}: #{key} -> #{load} watts"
  meter = db.meters.findOne key: key, (err, meter) ->
    if err
      console.log "[err] #{err}"
    unless meter
      console.log "[data] couldn't find corresponding meter"
    else
      db.meters.update meter, $set: {currentWatt: load, updated: new Date}
      db.signals.save time: new Date(), meter: meter._id, current: load

server.bind SERVER_PORT

http = require "http"
connect = require "connect"
connectQuery = require "connect-query"

app = connect()
  .use(connectQuery())
  .use (req, res) ->
    result = {}
    meter = db.meters.findOne name: req.query.device, (err, meter) ->
      if meter
        result = {current: meter.currentWatt}
    res.end JSON.stringify result

http.createServer(app).listen 3000