"use strict"

var _fs  =require("fs"),
    _path=require("path"),
    _dmx =require(__dirname + "/build/Release/dmxpro.node")

/**
 * dmxlight, holds configuration for one rgb light source
 */
var dmxlight = {
    init: function(path) {
        var light = JSON.parse(_fs.readFileSync(path));
        light.actions = []
        light.current = null
        return light;
    }
};

/**
 * dmxchannel, resolves channel mapping and such things
 */
var dmxchannel = {
    // input rgb, get channels
    rgb: function(light, red, green, blue) {
        if (typeof light.map["red"] !== "undefined" &&
            typeof light.map["green"] !== "undefined" && 
            typeof light.map["blue"] !== "undefined")
            return [
                [light.map["red"]  , red   * 255],
                [light.map["green"], green * 255],
                [light.map["blue"] , blue  * 255]];
        else
            return [
                [-1, 0], [-1, 0], [-1, 0]];
    },
    // input intensity get channel
    intensity: function(light, intensity) {  
        if (typeof light.map["intensity"] !== "undefined")
            return [[light.map["intensity"], intensity * 255]];
        return [[-1, 0]];
    },
    map: function(light, struct) {
        var k,i,chan=[],keys = Object.keys(struct)
        
        for (i=0 ; i < keys.length ; i++) {
            k = keys[i]
            if (typeof light.map[k] == undefined)
                continue;
            chan.push([light.map[k], struct[k] * 255])
        }
        return chan
    },
    post: function(dmx, chan) {
        var i,k                
        if (dmx.io == null)
            return         
         for (i=0 ; i < chan.length ; i++) {
             k = chan[i];
             if (k.length == 2 && k[0] != -1) {
                 dmx.io.queue(k[0], k[1])
             }
         }
    },
    read: function(dmx, light) {
        var r=0,g=0,b=0,i=0,m=light.map;
        
        if (dmx.io == null)
            return {red: 0, green: 0, blue: 0, intensity: 0}
        if (typeof m["red"] !== "undefined")
            r = (dmx.io.get(m["red"]) / 255.0)
        if (typeof m["green"] !== "undefined") 
            g = (dmx.io.get(m["green"]) / 255.0)
        if (typeof m["blue"] !== "undefined")
            b = (dmx.io.get(m["blue"]) / 255.0)
        if (typeof m["intensity"] !== "undefined")
            i = (dmx.io.get(m["intensity"]) / 255.0)
        return {red: r, green: g, blue: b, intensity: i}
    }    
};

/**
 * dmxaction, common fields for actions
 */
var dmxaction = {
    alloc: function() {
        return {
            time: 0.0,
            length: 0.0,
        }
    }
};

/**
 * dmxfade, fade values over time
 */
var dmxfade = {
    rgb: function(r, g, b, duration) {
        var a = dmxaction.alloc()           
        a.source = {red:0, green:0, blue:0};
        a.value  = {red:0, green:0, blue:0};
        a.delta  = {red:0, green:0, blue:0};
        a.target = {red:r, green:g, blue:b};
        a.length = duration;
        a.step    = function() {
            this.value.red   = (this.source.red   + this.delta.red   * this.time);
            this.value.green = (this.source.green + this.delta.green * this.time);
            this.value.blue  = (this.source.blue  + this.delta.blue  * this.time);
        };
        a.init = function(c) {
            this.source      = {red:c.red, green:c.green, blue:c.blue};
            this.delta.red   = this.target.red   - this.source.red;
            this.delta.green = this.target.green - this.source.green;
            this.delta.blue  = this.target.blue  - this.source.blue;
        };
        return a;
    },
    intensity: function(i, duration) {
        var a = dmxaction.alloc()           
        a.source = {intensity:0};
        a.value  = {intensity:0};
        a.delta  = {intensity:0};
        a.target = {intensity:i};
        a.length = duration;
        a.step    = function() {
            this.value.intensity = (this.source.intensity + this.delta.intensity   * this.time);
        };
        a.init = function(c) {
            this.source = {intensity:c.intensity};
            this.delta.intensity = this.target.intensity - this.source.intensity;
        };
        return a;
    },
};

/**
 * dmxpulse, pulse values for duration
 */
var dmxpulse = {
    intensity: function(value, freq, duration) {
        var a = dmxaction.alloc()           
        a.freq   = freq;
        a.value  = {intensity:0};
        a.target = {intensity:value};
        a.length = duration;
        a.step    = function() {
            this.value.intensity = (this.source.intensity + this.delta.intensity   * this.time);
        };
        a.init = function(c) {
            this.source = {intensity:c.intensity};
        };
        return a;
    },    
};

/**
 * center of dmx universe
 */
var dmxmain = {
    time: function() {
        return (new Date()).getTime() / 1000.0;
    },
    // tick, step time for universe in dmx
    tick: function(dmx) {
        var now, a,i,light,names,ch,dt; 
             
        now = dmxmain.time()

        dt  = now - dmx.lasttick;

        if (dmx.io == null)
            return ;

        names = Object.keys(dmx.lights)
        
        for (i=0 ; i < names.length ; i++) {            
            light = dmx.lights[names[i]];
            if (light.current != null) {
                a = light.current;
                if (a.time < 1.0) {
                    a.step();
                    ch = dmxchannel.map(light, a.value);                    
                    dmxchannel.post(dmx, ch);
                    a.time += (dt / a.length);
                } else {
                    light.current = null;
                }
            } else {                
                if (light.actions.length > 0) {
                    light.current = light.actions[0]               
                    light.actions.shift()
                    light.current.init(dmxchannel.read(dmx, light))
                }
            }
        }
        dmx.io.flush();
        dmx.lasttick = now;
    }
};

/**
 * main interface
 */
var dmxpro = {
    RATE: 1.0/30.0,
    // create new empty universe
    alloc: function() {
        return {
            io: null,
            configs: {},
            interval: null,
            lasttick: 0,
            colorspace: "rgb",
            scale: 1.0,
            lights: {}
        };
    },
    // init universe and populate from dmx.json in path, big bang
    init: function(dmx, path) {
        var conf,light,light_path,dir,i;
        conf = JSON.parse(_fs.readFileSync(path))
        if (dmx.io != null)
            throw "Can not setup dmxpro interface, already setup";
        dmx.io = new _dmx.io();
        // call native open, since device may not 
        // be defined in config, we need to call
        // open without args to get det autoselect behavior
        if (typeof conf.device !== "undefined" && conf.device != "auto") {
            if (!dmx.io.open(conf.device))
                throw "Could not open device!";
        } else {
            if (!dmx.io.open())
                throw "Could not open device! (autoselect mode)";
        } 
        if (typeof conf.colorspace !== "undefined") {
            dmx.colorspace = conf.colorspace;        
        }
        if (typeof conf.range !== "undefined") {
            dmx.scale = 1.0/conf.range;
        }
        if (typeof conf.lights !== "undefined") {
            light_path = _path.dirname(path) + "/" + conf.lights;
            dir = _fs.readdirSync(light_path)
            for (i=0 ; i < dir.length ; i++) {
                light = dmxlight.init(light_path + "/" + dir[i])
                if (typeof light.name !== "undefined")
                    dmx.lights[light.name] = light
            }
            console.log("dmxpro.setup() " + Object.keys(dmx.lights).length + " lights in universe")
        }
        return true
    },    
    // start universe ticks
    start: function(dmx) {
        if (dmx.interval == null) { 
            dmx.lasttick = dmxmain.time()
            dmx.interval = setInterval(dmxmain.tick, dmxpro.RATE*1000, dmx);
            console.log("started!")
        }
    },
    // shutdown, please remember to do this
    shutdown: function(dmx) {
        if (dmx.interval != null)
            clearInterval(dmx.interval)
        dmx.interval = null
        if (dmx.io != null) {
            dmx.io.blackout()
            dmx.io.wait(0.2)
            dmx.io.close()   
        }
        dmx.io = null
    },
    lights: function(dmx) {
        return Object.keys(dmx.lights);
    },
    // makes everything black, or white if already black (it's a toggle)
    blackout: function(dmx) {
        dmx.io.blackout()
    },
    black: function(dmx) {
        var i=0;
        for (i=0 ; i < 512 ; i++)
            dmx.io.queue(i, 0);
        dmx.io.flush();
    },
    // handle dmx pseudo message
    message: function(dmx, m) {
        var light=null,color=null,c,ch;
        
        if (typeof m.fixtures == "undefined") {
            console.log("message struct missing fixtures property, aborting!")
            return ;
        }
        if (typeof m["color"] != "undefined")            
            color = m["color"];
        else if (typeof m["colour"] != "undefined")
            color = m["colour"];
        
        if (color == null) {
            console.log("message struct missing both color and or colour property, aborting!")
            return ;
        }
                
        m.fixtures.forEach(function(light_name, index, obj) {
        
            if (typeof dmx.lights[light_name] !== "undefined") {
            
                light = dmx.lights[light_name];
    
                c = dmxchannel.read(dmx, light);
    
                Object.keys(c).forEach(function(prop, index, obj) {
                    if (typeof color[prop] !== "undefined")
                        c[prop] = color[prop];
                });
    
                if (typeof m.fade !== "undefined") {
                    dmxpro.color(dmx, m.light, c.red, c.green, c.blue, m.fade)
                } else {
                    ch = dmxchannel.rgb(light, c.red, c.green, c.blue);
                    dmxchannel.post(dmx, ch);                
                }                
            } else {
                console.log("No light named " + light_name + " in universe");
            }
        });
        dmx.io.flush();
    },
    // set light color, with optional fade time
    color: function(dmx, light_name, _red, _green, _blue, fade_time) {
        var fade,c,red=_red*dmx.scale, green=_green*dmx.scale, blue=_blue*dmx.scale,light=dmx.lights[light_name]        
        if (typeof fade_time == "undefined" || dmx.interval == null) {   
            c = dmxchannel.rgb(light, red, green, blue);
            dmxchannel.post(dmx, c);
            dmx.io.flush()
            return;
        }
        fade = dmxfade.rgb(red, green, blue, fade_time);
        light.actions.push(fade)
        return ;
    },
    // set light intensity with current color, with optional fade time
    intensity: function(dmx, light_name, _intensity, fade_time) {
        var fade,i,intensity=_intensity*dmx.scale,light=dmx.lights[light_name]        
        if (typeof light.map["intensity"] == "undefined") {
            i = dmxchannel.read(light)
            return dmxpro.color(dmx, light_name, i.red * intensity, i.green * intensity, i.blue * intensity, fade_time);
        }        
        if (typeof fade_time == "undefined" || dmx.interval == null) {   
            i = dmxchannel.intensity(light, intensity)
            dmxchannel.post(dmx, i);
            dmx.io.flush();
            return ;
        } 
        fade = dmxfade.intensity(intensity, fade_time);
        light.actions.push(fade)
        return ;
    }    
};

module.exports=dmxpro