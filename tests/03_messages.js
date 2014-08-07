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

/*
unit.run('red message', function() {    
  unit.assert(dmx != null);    
  var msg = {
    colour: {
      red: 1, 
      blue: 0,
      green: 0
    } 
  };

  dmxpro.message(dmx, 1, 1.0, 0.0, 0.0)
  delay();
});
*/

unit.run('stop', function() {
  unit.assert(dmx != null)
  console.log("waiting for test to finish")
  setTimeout(function() {
      // finalize and shutdown..
      dmxpro.shutdown(dmx);
  }, 10000);
});
