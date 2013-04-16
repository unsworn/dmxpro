"use strict"
var dmxpro=require(__dirname + "/dmxpro.js"),
    dmx=null,
    lights=null,
    light=null,
    itime=0,
    i=0;

dmx = dmxpro.alloc()

console.log("Starting fader")

if (!dmxpro.init(dmx, __dirname + "/tests/data/dmx.json"))
    process.exit()

dmxpro.start(dmx)

console.log("Opened device")

lights = dmxpro.lights(dmx);

for (i=0 ; i < lights.length ; i++) {
    
    light = lights[i];
    
    console.log("Making Fader Test: " + light);
        
    setTimeout(function(dmx, target) {
        
        console.log("Fading ==> " + target);
        
        dmxpro.color(dmx, target, 0.0, 0.0, 0.0, 0.1);
        dmxpro.color(dmx, target, 1.0, 0.0, 0.0, 1.0);
        
        dmxpro.color(dmx, target, 0.0, 0.0, 0.0, 1.0);
        dmxpro.color(dmx, target, 0.0, 1.0, 0.0, 1.0);
        
        dmxpro.color(dmx, target, 0.0, 1.0, 1.0, 1.0);
        dmxpro.color(dmx, target, 0.0, 0.0, 0.0, 1.0);
    
    }, itime, dmx, light);
    
    itime += 5000;
}

setTimeout(function(dmx) {
    console.log("Completed");
    dmxpro.black(dmx);
    dmx.io.wait(0.5)
    dmxpro.shutdown(dmx);
}, itime + 5000, dmx);



console.log("Started");
