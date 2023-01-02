import urllib2
import json
import pprint
import struct
import usb1
import time

###############################################################################

# http://cph-cgt-build1.eidos.com:1080/json/builders/?as_text=1
# http://cph-cgt-build1.eidos.com:1080/json/builders/ninja(nacl)/builds/-1?as_text=1

base_url = 'http://cph-cgt-build1.eidos.com:1080/json/'

###############################################################################

def find_dev(manifacture, product):
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

###############################################################################

important_builds = (
  'angle',
  'assetmanager',
  'cookers',
  'debug',
  'nacl-release',
  'release',
  'trusted-release',
  'assets',
#  'ninja(nacl)',
)

def get_build_status():
  '''return two bools (is_building, has_failed)'''
  has_failed = False
  is_building = False

  builders = json.loads(urllib2.urlopen(base_url + 'builders/').read())
  #pprint.pprint(builders)
  for build_name in important_builds:
    assert build_name in builders

    builds = json.loads(urllib2.urlopen(base_url + 'builders/%s/builds?select=-1&select=-2'%build_name).read())
    #pprint.pprint(builds)

    build = builds.get('-1')

    if build:
      #print 'STEP', build_name, build['currentStep']
      if build['currentStep']:
        is_building = True
        build = builds.get('-2') # check prev build for failure/success

    if build:
      if 'failed' in build['text']:
        has_failed = True
      elif 'exception' in build['text']:
        has_failed = True
      elif 'successful' in build['text']:
        pass
      else:
        print 'Unknown text', build['text']

  return is_building, has_failed

build_idle_good = LightProg()
build_idle_good.load_delay(2500)
build_idle_good.load_rgb(0, 31, 0)
build_idle_good.go()
build_idle_good.load_rgb(0, 20, 0)
build_idle_good.go()
build_idle_good.jump(0)

build_building_good = LightProg()
build_building_good.load_delay(500)
build_building_good.load_rgb(10, 31, 0)
build_building_good.go()
build_building_good.load_delay(200)
build_building_good.go()
build_building_good.load_rgb(10, 20, 0)
build_building_good.go()
build_building_good.load_delay(200)
build_building_good.go()
build_building_good.jump(0)

build_idle_failed = LightProg()
build_idle_failed.load_rgb(31, 0, 0)
build_idle_failed.load_delay(1)
build_idle_failed.go()
build_idle_failed.load_delay(200)
build_idle_failed.go()
build_idle_failed.load_rgb(0, 0, 0)
build_idle_failed.load_delay(100)
build_idle_failed.go()
build_idle_failed.load_delay(200)
build_idle_failed.go()
build_idle_failed.jump(0)

build_building_failed = LightProg()
build_building_failed.load_delay(500)
build_building_failed.load_rgb(31, 21, 0)
build_building_failed.go()
build_building_failed.load_rgb(20, 10, 0)
build_building_failed.go()
build_building_failed.jump(0)

last_col = None

def check_build():
  is_building, has_failed = get_build_status()
  #print is_building, has_failed

  dev = find_dev('BitHeap', 'ColorThing')
  #print dev.getConfiguration()
  #dev.claimInterface(0)
  #dev.setInterfaceAltSetting(0, 0)

  if is_building:
    if has_failed:
      col = build_building_failed
    else:
      col = build_building_good
  else:
    if has_failed:
      col = build_idle_failed
    else:
      col = build_idle_good

  global last_col
  if col != last_col:
    dev.controlWrite(1<<5, 0, 0, 0, col.compile())
    last_col = col


def main():
  while True:
    check_build()
    time.sleep(10)


main()

