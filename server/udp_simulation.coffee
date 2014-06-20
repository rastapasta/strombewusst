###******************************
*  StromBewusst - UDP Simulator
# *******************************
###

dgram = require "dgram"
message = new Buffer "\xAA\xBB\xCC\xDD\xEE\xFF\x3C\x01"
client = dgram.createSocket "udp4"

client.send message, 0, message.length, 8888, "localhost", (err, bytes) ->
  client.close()
