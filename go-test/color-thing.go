package main

import (
	"fmt"
	"github.com/google/gousb"
	"log"
)

// openColorThing find the first "ColorThing" and returns it.
func openColorThing(ctx *gousb.Context) (*gousb.Device, error) {
	if devs, err := ctx.OpenDevices(func(desc *gousb.DeviceDesc) bool {
		log.Printf("USB Device: %04x:%04x\n", desc.Device, desc.Product)
		return desc.Vendor == 0x16c0 && desc.Product == 0x05dc
	}); err != nil {
		return nil, err
	} else if len(devs) == 0 {
		return nil, fmt.Errorf("no libusb devices found")
	} else {
		var colorThing *gousb.Device

		for _, dev := range devs {
			if colorThing == nil {
				if manufacturer, err := dev.Manufacturer(); err != nil {
					log.Printf("Failed to get manufacturer for device %s: %s\n", dev, err)
				} else if product, err := dev.Product(); err != nil {
					log.Printf("Failed to get product for device %s: %s\n", dev, err)
				} else if manufacturer == "BitHeap" && product == "ColorThing" {
					colorThing = dev
					continue
				}
			}
			_ = dev.Close()
		}

		if colorThing == nil {
			return nil, fmt.Errorf("no ColorThing devices found")
		}

		return colorThing, nil
	}
}
