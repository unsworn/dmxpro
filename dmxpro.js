fs=require("fs")
_dmx=require(__dirname + "/build/Release/dmxpro.node")

var dmxpro = {
    alloc: function() {
        return {
            channels: null;
            configs: {};
            
        };
    },
    setup: function(d, path) {
        conf = JSON.parse(fs.readFileSync(path))
        
        d.channels.open
        
    }
};
module.exports=dmxpro