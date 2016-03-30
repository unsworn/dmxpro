#include "DmxPro.h"
#include <node_buffer.h>

#include <sys/stat.h>
#include <string>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

using namespace std;
using namespace node;
using namespace v8;

Persistent<Function> DmxPro::constructor;

DmxPro::DmxPro(Local<Object> wrapper) :
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
DmxPro::init(Local<Object> target) {
    Isolate* isolate = target->GetIsolate();

    Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, DmxPro::New);

    tpl->SetClassName(String::NewFromUtf8(isolate, "io"));
    tpl->InstanceTemplate()->SetInternalFieldCount(1);

    NODE_SET_PROTOTYPE_METHOD(tpl, "open"    , open);
    NODE_SET_PROTOTYPE_METHOD(tpl, "close"   , close);
    NODE_SET_PROTOTYPE_METHOD(tpl, "write"   , write);
    NODE_SET_PROTOTYPE_METHOD(tpl, "blackout", blackout);
    NODE_SET_PROTOTYPE_METHOD(tpl, "set"     , set);
    NODE_SET_PROTOTYPE_METHOD(tpl, "get"     , get);
    NODE_SET_PROTOTYPE_METHOD(tpl, "queue"   , queue);
    NODE_SET_PROTOTYPE_METHOD(tpl, "flush"   , flush);
    NODE_SET_PROTOTYPE_METHOD(tpl, "wait"    , wait);

    constructor.Reset(isolate, tpl->GetFunction());

    target->Set(String::NewFromUtf8(isolate, "io"), tpl->GetFunction());
}

void
DmxPro::New(const FunctionCallbackInfo<Value>& args) {
    DmxPro* dmx = new DmxPro(args.This());
    dmx->Wrap(args.This());
    args.GetReturnValue().Set(args.This());
}

void
DmxPro::open(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();
    DmxPro* dmx = ObjectWrap::Unwrap<DmxPro>(args.This());

    struct stat st;
    string dev;
    serialinfo_list_s list;
    int i;

    if (dmx->port != NULL) {
        fprintf(stderr, "io::open() already open\n");
        args.GetReturnValue().Set(Boolean::New(isolate, true));
        return;
    }

    if (args.Length() >= 1) {

        String::Utf8Value path(args[0]);

        if ( ((const char*) *path)[0] == '/' && stat(*path, &st) != 0) {
            fprintf(stderr, "io::open() no such file or directory: %s\n", *path);
            args.GetReturnValue().Set(Boolean::New(isolate, false));
            return;
        }

        dev = *path;
    }
    else
    {
        serial_list(&list);

        if (list.size == 0) {
            fprintf(stderr, "io::open() no serial port found\n");
            args.GetReturnValue().Set(Boolean::New(isolate, false));
            return;
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
        args.GetReturnValue().Set(Boolean::New(isolate, false));
        return;
    }

    dmx->port = (serial_s*) malloc( sizeof(serial_s) );

    serial_init(dmx->port);

    if (dmx->port == NULL) {
        fprintf(stderr, "io::open() failed, out of memory\n");
        args.GetReturnValue().Set(Boolean::New(isolate, false));
        return;
    }

    //printf("io::open() %s\n", dev.c_str());

    args.GetReturnValue().Set(Boolean::New(isolate,  serial_open(dmx->port, dev.c_str()) == SERIAL_OK ));
}

void
DmxPro::close(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();
    DmxPro* dmx = ObjectWrap::Unwrap<DmxPro>(args.This());

    if (dmx->port == NULL) {
        fprintf(stderr, "io::close() already closed\n");
        args.GetReturnValue().Set(Boolean::New(isolate, false));
        return;
    }

    fprintf(stderr, "io::close() shutdown dmx\n");

    if (serial_isopen(dmx->port)) {
        serial_close(dmx->port);
    }

    if (dmx->port != NULL) {
        free(dmx->port);
    }

    dmx->port = NULL;

    args.GetReturnValue().Set(Boolean::New(isolate, true));
}

void
DmxPro::write(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();
    DmxPro* dmx = ObjectWrap::Unwrap<DmxPro>(args.This());

    const char* bytes = NULL;
    int size = 0;

    if (args.Length() < 1) {
        fprintf(stderr, "io::write() needs atleast 1 argument\n");
        args.GetReturnValue().Set(Boolean::New(isolate, false));
        return;
    }

    if (!Buffer::HasInstance(args[0])) {
        fprintf(stderr, "io::write() first argument needs to be of type buffer\n");
        args.GetReturnValue().Set(Boolean::New(isolate, false));
        return;
    }

    if (dmx->port == NULL) {
        fprintf(stderr, "io::write() not opened\n");
        args.GetReturnValue().Set(Boolean::New(isolate, false));
        return;
    }

    size  = Buffer::Length(args[0]->ToObject());
    bytes = Buffer::Data(args[0]->ToObject());

    if (bytes == NULL) {
        fprintf(stderr, "io::write() unpack failed\n");
        args.GetReturnValue().Set(Boolean::New(isolate, false));
        return;
    }

    if (size > DMX_UNIVERSE_SIZE)
        size = DMX_UNIVERSE_SIZE;

    memcpy(dmx->state, bytes, size);

    args.GetReturnValue().Set(Boolean::New(isolate, dmx->drain()));
}

void
DmxPro::blackout(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();
    DmxPro* dmx = ObjectWrap::Unwrap<DmxPro>(args.This());

    if (dmx->port == NULL) {
        fprintf(stderr, "io::set() not openend, can not set\n");
        args.GetReturnValue().Set(Boolean::New(isolate, false));
        return;
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

    args.GetReturnValue().Set(Boolean::New(isolate, dmx->drain()));
}

void
DmxPro::set(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();
    DmxPro* dmx = ObjectWrap::Unwrap<DmxPro>(args.This());

    int pos=0,val=0;

    if (dmx->port == NULL) {
        fprintf(stderr, "io::set() not opened, can not set\n");
        args.GetReturnValue().Set(Boolean::New(isolate, false));
        return;
    }

    if (args.Length() < 2) {
        fprintf(stderr, "io::set() requires two internal::arguments (channel, value)\n");
        args.GetReturnValue().Set(Boolean::New(isolate, false));
        return;
    }

    pos = args[0]->Int32Value();
    val = args[1]->Int32Value();

    dmx->state[pos%DMX_UNIVERSE_SIZE] = (val&0xFF);

    args.GetReturnValue().Set(Boolean::New(isolate, dmx->drain()));
}

void
DmxPro::get(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();
    DmxPro* dmx = ObjectWrap::Unwrap<DmxPro>(args.This());

    int pos=0,val=0;

    if (dmx->port == NULL) {
        fprintf(stderr, "io::get() not started, can not set\n");
        args.GetReturnValue().Set(Integer::NewFromUnsigned(isolate, 0));
        return;
    }

    if (args.Length() < 1) {
        fprintf(stderr, "io::get() requires one internal::arguments (channel)\n");
        args.GetReturnValue().Set(Integer::NewFromUnsigned(isolate, 0));
        return;
    }

    pos = args[0]->Int32Value();

    val = dmx->state[pos%DMX_UNIVERSE_SIZE];

    args.GetReturnValue().Set(Integer::NewFromUnsigned(isolate, val));
}

void
DmxPro::queue(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();
    DmxPro* dmx = ObjectWrap::Unwrap<DmxPro>(args.This());
    int pos=0,val=0;

    if (dmx->port == NULL) {
        fprintf(stderr, "io::queue() not opened, can not queue\n");
        args.GetReturnValue().Set(Boolean::New(isolate, false));
        return;
    }

    if (args.Length() < 2) {
        fprintf(stderr, "io::queue() requires two internal::arguments (channel, value)\n");
        args.GetReturnValue().Set(Boolean::New(isolate, false));
        return;
    }

    pos = args[0]->Int32Value();
    val = args[1]->Int32Value();

    dmx->state[pos%DMX_UNIVERSE_SIZE] = (val&0xFF);

    args.GetReturnValue().Set(Boolean::New(isolate, true));
}

void
DmxPro::flush(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();
    DmxPro* dmx = ObjectWrap::Unwrap<DmxPro>(args.This());

    if (dmx->port == NULL) {
        fprintf(stderr, "io::set() not opened, can not flush\n");
        args.GetReturnValue().Set(Boolean::New(isolate, false));
        return;
    }

    args.GetReturnValue().Set(Boolean::New(isolate, dmx->drain()));
}

void
DmxPro::wait(const FunctionCallbackInfo<Value>& args) {
    int slice = 500;

    if (args.Length() > 0) {
        double frac = args[0]->NumberValue();
        slice = frac * 1000;
    }

    usleep( slice * 1000 );

    args.GetReturnValue().Set(Integer::NewFromUnsigned(args.GetIsolate(), 0));
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
    static void init_dmxpro(Local<Object> target) {
        DmxPro::init(target);
    }
    NODE_MODULE(dmxpro, init_dmxpro);
}
