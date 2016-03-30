#ifndef DmxPro_H_
#define DmxPro_H_

#include <node_object_wrap.h>
#include <v8.h>

#include "Serial.h"

#define DMX_UNIVERSE_SIZE     512
#define DMX_PACKET_SIZE       DMX_UNIVERSE_SIZE + 5
#define DMX_PRO_HEADER_SIZE   4
#define DMX_PRO_MESSAGE_START 0x7E
#define DMX_PRO_MESSAGE_END   0xE7
#define DMX_PRO_SEND_PACKET   0x06

using namespace v8;

class DmxPro : public node::ObjectWrap {
public:
    static void init(Local<Object> target);

    static void New     (const FunctionCallbackInfo<Value>& args);
    static void open    (const FunctionCallbackInfo<Value>& args);
    static void close   (const FunctionCallbackInfo<Value>& args);
    static void write   (const FunctionCallbackInfo<Value>& args);
    static void blackout(const FunctionCallbackInfo<Value>& args);
    static void set     (const FunctionCallbackInfo<Value>& args);
    static void get     (const FunctionCallbackInfo<Value>& args);
    static void queue   (const FunctionCallbackInfo<Value>& args);
    static void flush   (const FunctionCallbackInfo<Value>& args);
    static void wait    (const FunctionCallbackInfo<Value>& args);

protected:
    ~DmxPro();

private:
    DmxPro(Local<Object> target);
    static Persistent<Function> constructor;

    bool drain();

    unsigned char saved [DMX_UNIVERSE_SIZE];
    unsigned char state [DMX_UNIVERSE_SIZE];

    bool          bo;
    serial_s*     port;
};

#endif
