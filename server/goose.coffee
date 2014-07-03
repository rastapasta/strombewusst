mongoose = require "mongoose"
mongoose.connect "mongodb://localhost/strombewusst"

meterSchema = mongoose.Schema
  key: String,
  updated: Date,
  currentWatt: Number,
  name: String,
  description: String,
  price: Number,
  spark: String

signalSchema = mongoose.Schema
  time: Number,
  meter: mongoose.Schema.Types.ObjectId,
  current: Number

Meter = mongoose.model('meters', meterSchema)
Signal = mongoose.model('signals', signalSchema)

Meter.findOne (err, signals) ->
	console.log signals
