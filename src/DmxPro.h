#ifndef DmxPro_H_
#define DmxPro_H_

#include <node.h>
#include <v8.h>

#include "Serial.h"

#define DMX_UNIVERSE_SIZE     512
#define DMX_PACKET_SIZE       DMX_UNIVERSE_SIZE + 5
#define DMX_PRO_HEADER_SIZE   4
#define DMX_PRO_MESSAGE_START 0x7E
#define DMX_PRO_MESSAGE_END   0xE7
#define DMX_PRO_SEND_PACKET   0x06

using namespace v8;

class DmxPro : node::ObjectWrap {
public:
    static void init(Handle<Object> target);

    static Handle<Value> New     (const Arguments& args);
    static Handle<Value> open    (const Arguments& args);
    static Handle<Value> close   (const Arguments& args);
    static Handle<Value> write   (const Arguments& args);
    static Handle<Value> blackout(const Arguments& args);
    static Handle<Value> set     (const Arguments& args);
    static Handle<Value> get     (const Arguments& args);
    static Handle<Value> wait    (const Arguments& args);
    
protected:
    ~DmxPro();

private:
    DmxPro(Handle<Object> wrapper);
    static Persistent<FunctionTemplate> constructor_template;    
    
    bool flush();
    
    unsigned char saved [DMX_UNIVERSE_SIZE];
    unsigned char state [DMX_UNIVERSE_SIZE];
    
    bool          bo;
    serial_s*     port;
};

#endif
