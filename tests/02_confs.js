var unit = require('./units');
var dmxpro = require('../dmxpro');

var dmx = null;

function delay(t) { 
  if (typeof t == "undefined")
    t = 0.5
 if (dmx != null)
    dmx.io.wait(t)
}

unit.run('begin', function() {
    
    // new universe
    dmx = dmxpro.alloc()
    
    unit.assert(dmx != null)
    
    // init with settings from path
    dmxpro.initWithFile(dmx, __dirname + "/dmx.json")
});

unit.run('red', function() {    
  unit.assert(dmx != null);    
  dmxpro.color(dmx, 1, 1.0, 0.0, 0.0)
  delay();
});

unit.run('green', function() {    
    unit.assert(dmx != null);    
    dmxpro.color(dmx, 1, 0.0, 1.0, 0.0)
    delay()
});

unit.run('blue', function() {    
    unit.assert(dmx != null);    
    dmxpro.color(dmx, 1, 0.0, 0.0, 1.0)
    delay()
});

unit.run('mix', function() {    
    unit.assert(dmx != null);    
    dmxpro.color(dmx, 1, 0.6, 0.2, 0.5)
    delay()
});

unit.run('black', function() {    
    unit.assert(dmx != null);    
    dmxpro.color(dmx, 1, 0.6, 0.2, 0.5)
    delay()
});
   
unit.run('fade_red', function() {
    unit.assert(dmx != null);    

    dmxpro.color(dmx, 1, 0.0, 0.0, 0.0)
    
    dmxpro.start(dmx)        
    dmxpro.color(dmx, 1, 1.0, 0.0, 0.0, 1.0)
    dmxpro.color(dmx, 1, 0.0, 0.0, 0.0, 1.0)
});

unit.run('fade_green', function() {    
    unit.assert(dmx != null);    
    dmxpro.color(dmx, 1, 0.0, 1.0, 0.0, 0.5)
    dmxpro.color(dmx, 1, 0.0, 0.0, 0.0, 0.5)
});

unit.run('fade_blue', function() {    
    unit.assert(dmx != null);    
    dmxpro.color(dmx, 1, 0.0, 0.0, 1.0, 2.0)
    dmxpro.color(dmx, 1, 0.0, 0.0, 0.0, 1.0)
});

unit.run('fade_mix', function() {    
    unit.assert(dmx != null);        
    dmxpro.color(dmx, 1, 0.6, 0.2, 0.5, 0.1)
    dmxpro.color(dmx, 1, 1.0, 0.0, 1.0, 1.0)
    dmxpro.color(dmx, 1, 0.0, 0.0, 0.0, 1.0)
});
   
unit.run('pulse', function() {    
    unit.assert(dmx != null);        
    //dmxpro.strobe(dmx, "north", 1.0, 3.0)
});


unit.run('stop', function() {
    
  unit.assert(dmx != null)
  console.log("waiting for test to finish")
  setTimeout(function() {
      // finalize and shutdown..
      dmxpro.shutdown(dmx);
  }, 10000);
});
