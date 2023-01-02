#!/usr/bin/env python
import serial
import sys
import time
import struct

verbose = True

class DebugWire:
  def __init__(self, device, speed):
    self.ser = serial.Serial(device, speed)

  def write_(self, data):
    if verbose: sys.stdout.write('  >')
    for c in data:
      if verbose: sys.stdout.write(' %02x'%ord(c))
      if verbose: sys.stdout.flush()
      self.ser.write(c)
      while True:
        rc = self.ser.read(1)
        if verbose: sys.stdout.write(' [%02x]'%ord(rc))
        if verbose: sys.stdout.flush()
        if rc == c: break
    if verbose: print

  def write(self, data):
    if verbose: sys.stdout.write('  > ' + ' '.join(' %02x'%ord(c) for c in data))
    if verbose: sys.stdout.flush()
    self.ser.write(data)
    if verbose: sys.stdout.write(' | ')
    if verbose: sys.stdout.flush()
    rdata = self.ser.read(len(data))
    if verbose: sys.stdout.write(' '.join(' %02x'%ord(c) for c in rdata))
    assert data == rdata
    if verbose: print

  def read(self, recvlen=1):
    if verbose: sys.stdout.write('  <')
    if verbose: sys.stdout.flush()
    data = ''
    while len(data) < recvlen:
      c = self.ser.read(1)
      if verbose: sys.stdout.write(' %02x'%ord(c))
      if verbose: sys.stdout.flush()
      data += c
    if verbose: print
    return data

  def break_(self):
    print 'break'
    self.ser.sendBreak()
    while self.read(1) != '\x55':
      pass

  def get_signature(self):
    print 'get_signature'
    self.write('\xf3')
    return self.read(2)

  def set_registers(self, index, values):
    print 'set_registers', index, values
    assert index+len(values) <= 0x20
    self.write('\x66')
    self.write(struct.pack('BBB', 0xd0, 0, index))
    self.write(struct.pack('BBB', 0xd1, 0, index+len(values)))
    self.write('\xc2\x05')
    self.write('\x20')
    self.write(struct.pack('%dB'%len(values), *values))


def read_ihex(filename):
  # || - data bytes
  #   |||| - address
  #       || - 00=data record
  #         |||||||||||||||||||||||||||||||| - data
  #                                         || - checksum
  #:1003F00032E017B31560C09A08B317BB55E020E888
  lastaddress = 0
  for line in open(filename).xreadlines():
    assert line[0] == ':'
    data = [int(line[i:i+2], 16) for i in range(1,len(line)-2,2)]
    assert data[0] == len(data)-5, data
    assert (-sum(data[0:-1]))&0xff == data[-1], data
    address = data[1]*256 + data[2]
    rectype = data[3]
    if rectype == 0:
      assert address >= lastaddress
      lastaddress = address + data[0]
      yield address, ''.join(chr(c) for c in data[4:-1])
    elif rectype == 1:
      break
    else:
      xxx

pages = {}

# "pagify" the input data
for addr, data in read_ihex(sys.argv[1]):
  #assert (addr&~63) ==  ((addr+len(data)-1)&~63), (addr, len(data)) # todo: split ihex records into pages if needed
  while len(data) > 0:
    pagei = addr&~63
    pageoff = addr&63
    datalen = len(data)
    if pageoff+datalen > 64: datalen = 64-pageoff
    page = pages.get(pagei, '\xff'*64)
    pages[pagei] = page[:pageoff] + data[:datalen] + page[pageoff+datalen:]
    assert len(pages[pagei]) == 64, len(pages[pagei])
    addr += datalen
    data = data[datalen:]



if 0:
  #for speed in range(8500000, 10000000, 50000):
  for speed in range(8000000, 17000000, 100000):
    dw = DebugWire('/dev/ttyUSB0', speed/128)
    dw.ser.sendBreak()
    print speed,
    d = dw.read(2)
    if '\x55' in d: print '!!!!!!!!!!!!!!'
  xxx

attiny45 = '\x92\x06'
attiny85 = '\x93\x0b'


#speed = 14000000
#speed = 9000000
#speed = 16000000
speed = 16500000

dw = DebugWire('/dev/ttyUSB0', speed/128)
dw.break_()
sig = dw.get_signature()
assert sig in (attiny45, attiny85), repr(sig)

for addr, data in sorted(pages.items()):
  print 'flash page'
  dw.set_registers(26, (3, 1, 5, 20, addr&0xff, addr>>8))

  print 'erase page' # see section "19. Self-Programming the Flash" in ATtiny25/45/85 datasheet
  dw.write('\x64'+\
    '\xd2\x01\xcf\x23'+ # movw r24:r25, r30:r31
    '\xd2\xbf\xa7\x23'+ # out 0x37, r26 # SPMCSR, PGERS|SPMEN
    '\xd2\x95\xe8\x33') # spm

  dw.break_()
  print 'fill temp page'
  for i in range(0, len(data), 2):
    d1, d2 = data[i:i+2]
    dw.write('\x44'+\
      '\xd2\xb4\x02\x23'+d1+ # in r0, 0x22 # r0, DWDR
      '\xd2\xb4\x12\x23'+d2+ # in r1, 0x22 # r0, DWDR
      '\xd2\xbf\xb7\x23'+    # out 0x37, r27 # SPMCSR, SPMEN
      '\xd2\x95\xe8\x23'+    # spm
      '\xd2\x96\x32\x23')    # adiw r30:r31, 2

  print 'write page'
  dw.write(
    '\xd2\x01\xfc\x23'+ # movw r30:r31, r24:r25
    '\xd2\xbf\xc7\x23'+ # out 0x37, r28 # SPMCSR, PGWRT|SPMEN
    '\xd2\x95\xe8\x23') # spm

  dw.break_()

  dw.set_registers(30, (addr&0xff, addr>>8))

  print 'read page'
  dw.write(
    '\xd0\x00\x00'+
    '\xc2\x02'+
    '\xd1\x00\x80'
    '\x20')
  read_data = dw.ser.read(len(data))
  assert read_data == data, (repr(read_data), repr(data))


  #dw.break_()




# def reset():
#   print 'reset'
#   ser.write('\x07')
#   while read(1) != '\x55':
#     pass

# def go():
#   print 'go'
#   ser.write('\x30')
#   while read(1) != '\x55':
#     pass


# break_()
# #reset()

# send('\xf0') # get pc
# read(2)

# send('\x07') # reset
# break_()

# send('\xf0') # get pc
# read(2)

dw.write('\x07') # reset
dw.break_()
# #send('\x60') # unknown
dw.write('\xd0\x00\x00') # set pc
dw.write('\x30') # run
