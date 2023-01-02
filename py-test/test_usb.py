import sys
import usb1
import time
import struct

def findDev(manifacture, product):
  c = usb1.LibUSBContext()
  c.setDebug(9999)
  for dev in c.getDeviceList():
    if dev.getVendorID()==0x16c0 and dev.getProductID()==0x05dc:
      #print dev.getManufacturer(), dev.getProduct()
      if dev.getManufacturer()==manifacture and dev.getProduct()==product:
        return dev.open()
  raise RuntimeError, 'could not find usb device'


class LightProg:
  def __init__(self):
    self.prog = []

  def compile(self):
    return ''.join(struct.pack('<H',v) for v in self.prog)

  def load_rgb(self, r, g, b):
    assert r>=0 and r<32 and g>=0 and g<32 and b>=0 and b<32
    self.prog.append(0b0000000000000000 | (r<<10) | (g<<5) | (b<<0))

  def load_delay(self, delay):
    exp = 0
    while delay >= 1024:
      exp += 1
      delay >>= 1
    self.prog.append(0b1000000000000000 | (delay<<4) | exp)

  def go(self):
    self.prog.append(0b1111111111010000)

  def jump(self, addr):
    assert addr>=0 and addr<64
    self.prog.append(0b1110000100000000 | addr)

p = LightProg()
p.load_rgb(0, 0, 0)
p.load_delay(100)
p.go()

p.load_delay(200)
p.go()

p.load_rgb(31, 0, 0)
p.load_delay(1)
p.go()

p.load_delay(200)
p.go()
p.jump(0)

print ['0x%04x'%i for i in p.prog]

dev = findDev('BitHeap', 'ColorThing')
print dev.getConfiguration()
#dev.claimInterface(0)
#dev.setInterfaceAltSetting(0, 0)
print dev.controlWrite(1<<5, 0, 0, 0, p.compile())
