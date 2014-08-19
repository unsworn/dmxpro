"use strict"
var dmxpro=require(__dirname + "/dmxpro.js"),
dmx=null,
names=null,
light=null,
i=0,j=0;

dmx = dmxpro.alloc()

console.log("\nSmoketest!")

if (!dmxpro.initWithFile(dmx, __dirname + "/tests/dmx.json"))
    process.exit()

dmxpro.start(dmx)

names = Object.keys(dmx.fixtures)
      
for (i=0 ; i < names.length ; i++) {
  light = dmx.fixtures[names[i]];
  var m = {
    fixtures: [names[i]],
    color: {red:1.0, green:1.0, blue:1.0, intensity:1.0}
  };
  dmxpro.message(dmx, m);
}

dmx.io.flush();
console.log("Queue flushed, entering blocking mode");
dmx.io.wait(1.0);
console.log("Done. resuming shutdown");
dmxpro.black(dmx);
dmxpro.shutdown(dmx);

console.log("Done");
