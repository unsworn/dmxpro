#!/opt/local/bin/python

import sys

import time

from serial import *

DMX_PRO_MESSAGE_START=0x7E;
DMX_PRO_MESSAGE_END  =0xE7;
DMX_PRO_SEND_PACKET  =0x06;

class DmxIO(object):
    def __init__(self, path):
        self.path = path
        self.port = None
        self.universeSize = 6;
        self.channelValues = bytearray(self.universeSize);

    def setup(self):    
        self.port = Serial(self.path, 115200, timeout=1)    
        for i in range(0, self.universeSize):
            self.channelValues[i] = 0x0

    def getDMX(self, channel):
        if not channel in range(0, self.universeSize):
            return 0x0
        return self.channelValues[channel];

    def setDMX(self, channel, value):
        if self.channelValues[channel] != value:
            self.channelValues[channel] = int(value)
            msg = bytearray(self.universeSize+1)
            msg[0] = 0x0
            for i in range(0, self.universeSize):
                msg[i+1] = self.channelValues[i]
            self.sendDMX( DMX_PRO_SEND_PACKET, msg )

    def sendDMX(self, messageType, msg):
        size = len(msg)
        data = bytearray(size + 5)
        data[0] = DMX_PRO_MESSAGE_START
        data[1] = messageType
        data[2] = (size&0xFF)
        data[3] = ((size >> 8) & 0xFF)
        for i in range(0, size):
            data[i+4] = msg[i]
        data[size+4] = DMX_PRO_MESSAGE_END
        
        self.port.write(data)
        self.port.flush()
        
    def shutdown(self):
        try:
            self.port.close()
        except:
            pass

def main(args):
    print "Dmx Test"
    
    #io = DmxIO("/dev/cu.usbserial-AH019G6B")
    io = DmxIO("/dev/cu.usbserial-EN129148")
    
    io.setup()
    
    while True:
        for i in range(0, 255):
            io.setDMX(1, i)
            io.setDMX(2, i)
            io.setDMX(3, i)
            time.sleep(0.01)
        for i in range(255, 1): 
            io.setDMX(1, i)
            io.setDMX(2, i)
            io.setDMX(3, i)
            time.sleep(0.01)
            
        print "Wait"
        time.sleep(5)
        
    print ("Sent")
    
    time.sleep(2)
    
    io.shutdown()
    
if __name__ == "__main__":
    main(sys.argv)