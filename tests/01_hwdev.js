unit=require('./units');
dmxpro=require('../build/Release/dmxpro.node');

var io=null;

unit.run('connect', function() {
    
    io = new dmxpro.io()
    
    unit.assert(io != null)
    
    unit.assert(io.open())
    
    io.wait(0.5)    
    
});

unit.run('light_red', function() {
    
    unit.assert(io != null)
    
    io.set(2, 244)
    
    io.wait(0.1)
        
});

unit.run('light_red', function() {
    
    unit.assert(io != null)
    
    io.set(2, 0)
    io.set(3, 255)
    
    io.wait(0.1)
    
});

unit.run('blackout', function() {
    
    unit.assert(io != null)
    
    io.blackout()
    
    io.wait(0.5)
    
    unit.assert(io.close())
    
});


