#include "DmxPro.h"
#include <node_buffer.h>

#include <string>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

using namespace std;
using namespace node;
using namespace v8;

Persistent<FunctionTemplate> DmxPro::constructor_template;

DmxPro::DmxPro(Handle<Object> wrapper) :
bo(false),
port(NULL)
{
    memset(saved, 0, DMX_UNIVERSE_SIZE);
    memset(state, 0, DMX_UNIVERSE_SIZE);
}

DmxPro::~DmxPro()
{
    if (port != NULL) {
        fprintf(stderr, "DmxPro not closed properly\n");
        serial_close(port);
        delete port;
        port = NULL;
    }
}

void
DmxPro::init(Handle<Object> target) {
    HandleScope scope;

    Local<FunctionTemplate> t = FunctionTemplate::New(DmxPro::New);

    constructor_template = Persistent<FunctionTemplate>::New(t);

    constructor_template->InstanceTemplate()->SetInternalFieldCount(1);

    constructor_template->SetClassName(String::New("io"));

    NODE_SET_PROTOTYPE_METHOD(constructor_template, "open"    , open);
    NODE_SET_PROTOTYPE_METHOD(constructor_template, "close"   , close);
    NODE_SET_PROTOTYPE_METHOD(constructor_template, "write"   , write);
    NODE_SET_PROTOTYPE_METHOD(constructor_template, "blackout", blackout);
    NODE_SET_PROTOTYPE_METHOD(constructor_template, "set"     , set);
    NODE_SET_PROTOTYPE_METHOD(constructor_template, "get"     , get);
    NODE_SET_PROTOTYPE_METHOD(constructor_template, "queue"   , queue);
    NODE_SET_PROTOTYPE_METHOD(constructor_template, "flush"   , flush);
    NODE_SET_PROTOTYPE_METHOD(constructor_template, "wait"    , wait);
    
    target->Set(String::New("io"), constructor_template->GetFunction());
}

Handle<Value>
DmxPro::New(const Arguments& args) {
    HandleScope scope;
    DmxPro* dmx = new DmxPro(args.This());
    dmx->Wrap(args.This());
    return scope.Close(args.This());
}

Handle<Value>
DmxPro::open(const Arguments& args) {
    struct stat st;
    string dev;
    serialinfo_list_s list;
    int i;
    HandleScope scope;
    Local<Object> obj = args.This();
    DmxPro* dmx = ObjectWrap::Unwrap<DmxPro>(obj);

    
    if (dmx->port != NULL) {
        fprintf(stderr, "io::open() already open\n");
        return scope.Close(Boolean::New(true));
    }                   
    
    if (args.Length() >= 1) {
        
        String::Utf8Value path(args[0]);

        if ( ((const char*) *path)[0] == '/' && stat(*path, &st) != 0) {
            fprintf(stderr, "io::open() no such file or directory: %s\n", *path);
            return scope.Close(Boolean::New(false));
        }
        
        dev = *path;
    }
    else
    {
        serial_list(&list);
        
        if (list.size == 0) {
            fprintf(stderr, "io::open() no serial port found\n");
            return scope.Close(Boolean::New(false));
        }
        
        for (i=0 ; i < list.size ; i++) {            
            if (strstr(list.info[i].name, "usbserial-EN") != NULL) {
                dev.assign("/dev/");
                dev.append(list.info[i].name);
                fprintf(stderr, "selected %s\n", dev.c_str());
                break;
            }
        }
        
        serial_list_free(&list);
    }
    
    if (dev.empty()) {
        fprintf(stderr, "io::open() no serial port found/selected\n");
        return scope.Close(Boolean::New(false));
    }
    
    dmx->port = (serial_s*) malloc( sizeof(serial_s) );
    
    serial_init(dmx->port);
    
    if (dmx->port == NULL) {
        fprintf(stderr, "io::open() failed, out of memory\n");
        return scope.Close(Boolean::New(false));
    }
    
    //printf("io::open() %s\n", dev.c_str());
    
    return scope.Close(Boolean::New( serial_open(dmx->port, dev.c_str()) == SERIAL_OK ));
}

Handle<Value>
DmxPro::close(const Arguments& args) {   
    HandleScope scope;
    Local<Object> obj = args.This();
    DmxPro* dmx = ObjectWrap::Unwrap<DmxPro>(obj);

    if (dmx->port == NULL) {
        fprintf(stderr, "io::close() already closed\n");
        return scope.Close(Boolean::New(false));
    }
        
    fprintf(stderr, "io::close() shutdown dmx\n");
    
    if (serial_isopen(dmx->port)) {
        serial_close(dmx->port);
    }
    
    if (dmx->port != NULL) {
        free(dmx->port);
    }

    dmx->port = NULL;
    
    return scope.Close(Boolean::New(true));
}

Handle<Value>
DmxPro::write(const Arguments& args) {
    const char* bytes = NULL;
    int size = 0;
    
    HandleScope scope;    
    Local<Object> obj = args.This();
    DmxPro* dmx = ObjectWrap::Unwrap<DmxPro>(obj);
    
    if (args.Length() < 1) {
        fprintf(stderr, "io::write() needs atleast 1 argument\n");
        return scope.Close(Boolean::New(false));
    }

    if (!Buffer::HasInstance(args[0])) {
        fprintf(stderr, "io::write() first argument needs to be of type buffer\n");
        return scope.Close(Boolean::New(false));
    }
    
    if (dmx->port == NULL) {
        fprintf(stderr, "io::write() not opened\n");
        return scope.Close(Boolean::New(false));
    }
    
    
    size  = Buffer::Length(args[0]->ToObject());
    bytes = Buffer::Data(args[0]->ToObject());
    
    if (bytes == NULL) {
        fprintf(stderr, "io::write() unpack failed\n");
        return scope.Close(Boolean::New(false));
    }
    
    if (size > DMX_UNIVERSE_SIZE)
        size = DMX_UNIVERSE_SIZE;
        
    memcpy(dmx->state, bytes, size);
                    
    return scope.Close(Boolean::New(dmx->drain()));

} 

Handle<Value>
DmxPro::blackout(const Arguments& args) {   
    HandleScope scope;
    Local<Object> obj = args.This();
    DmxPro* dmx = ObjectWrap::Unwrap<DmxPro>(obj);
    
    if (dmx->port == NULL) {
        fprintf(stderr, "io::set() not openend, can not set\n");
        return scope.Close(Boolean::New(false));
    }
        
    if (dmx->bo) {
        memcpy(dmx->state, dmx->saved, DMX_UNIVERSE_SIZE);
        memset(dmx->saved, 0, DMX_UNIVERSE_SIZE);
        dmx->bo = false;
    } else {
        memcpy(dmx->saved, dmx->state, DMX_UNIVERSE_SIZE);
        memset(dmx->state, 0, DMX_UNIVERSE_SIZE);
        dmx->bo = true;
    }   
    
    return scope.Close(Boolean::New(dmx->drain()));
    
}

Handle<Value>
DmxPro::set(const Arguments& args) {   
    HandleScope scope;
    Local<Object> obj = args.This();
    DmxPro* dmx = ObjectWrap::Unwrap<DmxPro>(obj);
    int pos=0,val=0;
    
    if (dmx->port == NULL) {
        fprintf(stderr, "io::set() not opened, can not set\n");
        return scope.Close(Boolean::New(false));
    }
    
    if (args.Length() < 2) {
        fprintf(stderr, "io::set() requires two arguments (channel, value)\n");
        return scope.Close(Boolean::New(false));
    }
    
    
    pos = args[0]->Int32Value();
    val = args[1]->Int32Value();
    
    dmx->state[pos%DMX_UNIVERSE_SIZE] = (val&0xFF);

    return scope.Close(Boolean::New(dmx->drain()));
    
}

Handle<Value>
DmxPro::get(const Arguments& args) {   
    HandleScope scope;
    Local<Object> obj = args.This();
    DmxPro* dmx = ObjectWrap::Unwrap<DmxPro>(obj);
    int pos=0,val=0;
    
    if (dmx->port == NULL) {
        fprintf(stderr, "io::get() not started, can not set\n");
        return scope.Close(Integer::NewFromUnsigned(0));
    }
    
    if (args.Length() < 1) {
        fprintf(stderr, "io::get() requires one arguments (channel)\n");
        return scope.Close(Integer::NewFromUnsigned(0));
    }
        
    pos = args[0]->Int32Value();
        
    val = dmx->state[pos%DMX_UNIVERSE_SIZE];

    return scope.Close(Integer::NewFromUnsigned(val));    

}

Handle<Value>
DmxPro::queue(const Arguments& args) {   
    HandleScope scope;
    Local<Object> obj = args.This();
    DmxPro* dmx = ObjectWrap::Unwrap<DmxPro>(obj);
    int pos=0,val=0;
    
    if (dmx->port == NULL) {
        fprintf(stderr, "io::queue() not opened, can not queue\n");
        return scope.Close(Boolean::New(false));
    }
    
    if (args.Length() < 2) {
        fprintf(stderr, "io::queue() requires two arguments (channel, value)\n");
        return scope.Close(Boolean::New(false));
    }
    
    
    pos = args[0]->Int32Value();
    val = args[1]->Int32Value();
    
    dmx->state[pos%DMX_UNIVERSE_SIZE] = (val&0xFF);

    return scope.Close(Boolean::New(true));
    
}

Handle<Value>
DmxPro::flush(const Arguments& args) {   
    HandleScope scope;
    Local<Object> obj = args.This();
    DmxPro* dmx = ObjectWrap::Unwrap<DmxPro>(obj);
    
    if (dmx->port == NULL) {
        fprintf(stderr, "io::set() not opened, can not flush\n");
        return scope.Close(Boolean::New(false));
    }
    
    return scope.Close(Boolean::New(dmx->drain()));
    
}

Handle<Value>
DmxPro::wait(const Arguments& args) {   
    HandleScope scope;
    //Local<Object> obj = args.This();    
    //DmxPro* dmx = ObjectWrap::Unwrap<DmxPro>(obj);
    
    int slice = 500;
    
    if (args.Length() > 0) {
        double frac = args[0]->NumberValue();
        slice = frac * 1000;
    }
    
    usleep( slice * 1000 );
    
    return scope.Close(Integer::NewFromUnsigned(0));
    
}
bool
DmxPro::drain() {

    int           size = DMX_UNIVERSE_SIZE;
    unsigned char header[DMX_PRO_HEADER_SIZE];
    unsigned char eom = DMX_PRO_MESSAGE_END;
        
    header[0] = DMX_PRO_MESSAGE_START;
    header[1] = DMX_PRO_SEND_PACKET;
    header[2] = size&0xFF;
    header[3] = (size >> 8)&0xFF;
    
    if (port == NULL) {
        fprintf(stderr, "drain() aborted, invalid state\n");
        return false;
    }
    
    if (!serial_isopen(port)) {
        fprintf(stderr, "drain() port not ready\n");
        return false;
    }
    
    if (serial_write(port, header, DMX_PRO_HEADER_SIZE) != DMX_PRO_HEADER_SIZE) {
        fprintf(stderr, "drain() header data truncated\n");
        return false;
    }

    if (serial_write(port, state, DMX_UNIVERSE_SIZE) != DMX_UNIVERSE_SIZE) {
        fprintf(stderr, "drain() message data truncated\n");
        return false;
    }
    
    if (serial_write(port, &eom, 1) != 1) {
        fprintf(stderr, "drain() message failed\n");
        return false;
    }
         
    serial_drain(port);
    
    return true;    
}
extern "C" {
    static void init_dmxpro(Handle<Object> target) {
        DmxPro::init(target);
    }
    NODE_MODULE(dmxpro, init_dmxpro);
}       
