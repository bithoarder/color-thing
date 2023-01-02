package main

import (
	"image/color"
	"log"
	"time"
)

type LightProg struct {
	Code []uint8
}

func (p *LightProg) LoadColor(c color.Color) *LightProg {
	r, g, b, _ := c.RGBA()
	return p.LoadRGB(uint8(r>>8), uint8(g>>8), uint8(b>>8))
}

func (p *LightProg) LoadRGB(r, g, b uint8) *LightProg {
	return p.addInstr((0b0 << 15) | ((uint16(r) >> 3) << 10) | ((uint16(g) >> 3) << 5) | ((uint16(b) >> 3) << 0))
}

func (p *LightProg) LoadDelay(delay time.Duration) *LightProg {
	d := delay.Milliseconds()
	exp := 0
	for d >= 1024 {
		exp += 1
		d >>= 1
	}
	if exp > 15 {
		// clamp to max delay
		d = 0x3ff
		exp = 15
	}
	return p.addInstr((0b10 << 14) | uint16(d<<4) | uint16(exp))
}

func (p *LightProg) Run() *LightProg {
	return p.addInstr(0b1111111111010000)
}

func (p *LightProg) Jump(addr uint8) *LightProg {
	if addr >= 64 {
		panic("address out of bounds")
	}
	return p.addInstr(0b1110000100000000 | uint16(addr))
}

func (p *LightProg) addInstr(i uint16) *LightProg {
	log.Printf("Op: %04x\n", i)
	p.Code = append(p.Code, uint8(i&0xff), uint8(i>>8))
	return p
}
