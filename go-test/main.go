package main

import (
	"github.com/google/gousb"
	"log"
	"os"
	"time"
)

func main() {
	ctx := gousb.NewContext()
	defer ctx.Close()

	if colorThing, err := openColorThing(ctx); err != nil {
		log.Printf("Failed to open ColorThing: %s\n", err)
		os.Exit(1)
	} else {
		defer colorThing.Close()

		colorThing.Control(1<<5, 0, 0, 0, (&LightProg{}).
			LoadRGB(255, 0, 0).
			LoadDelay(100*time.Millisecond).
			Run(). // fade to red
			LoadDelay(500*time.Millisecond).
			Run(). // hold red
			LoadRGB(0, 255, 0).
			LoadDelay(100*time.Millisecond).
			Run(). // fade to green
			LoadDelay(500*time.Millisecond).
			Run(). // hold green
			LoadRGB(0, 0, 255).
			LoadDelay(100*time.Millisecond).
			Run(). // fade to blue
			LoadDelay(500*time.Millisecond).
			Run(). // hold blue
			Jump(0).Code)
	}
}
