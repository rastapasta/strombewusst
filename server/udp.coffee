###***************************
*  StromBewusst - UDP Server
# ****************************
# Configuration
###
SERVER_PORT = 8888
#***

dgram = require "dgram"
server = dgram.createSocket "udp4"

server.on "message", (msg, rinfo) ->
  data = new String msg
  
  # Decode message -> 16byte API key - 1byte framesize - 1byte impulses
  
  key = ''
  for i in [0..15]
    key += data.charCodeAt(i).toString(16);

  framesize = data.charCodeAt 16
  impulses = data.charCodeAt 17

  console.log "[data] #{rinfo.address}: #{key} -> #{impulses} impulses in #{framesize} seconds"

server.on "listening", () ->
  address = server.address()
  console.log "*** StromBewusst - UDP Server ***"
  console.log "[server] listening on #{address.address}:#{address.port}"

server.bind SERVER_PORT
