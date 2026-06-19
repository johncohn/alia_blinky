# Soldering the Alia Blinky PCB

This guide walks you through soldering the Seeed XIAO RP2040 microcontroller onto the Alia Blinky PCB. Soldering is optional — the board also works with the microcontroller plugged in via socket headers — but soldering it down gives a more secure, permanent connection.

**Time:** ~20 minutes  
**Skill level:** Beginner-friendly

## What You Need

- Alia Blinky PCB
- Seeed XIAO RP2040 (or compatible board)
- 2× 7-pin male header strips (usually included with the XIAO)
- Soldering iron (300°C / 570°F)
- Solder (rosin-core recommended)
- Isopropyl alcohol (IPA) + cotton swabs for cleanup
- Flush cutters + safety glasses for trimming pins
- Kapton or masking tape (optional, helps hold the chip while soldering the back)

---

## Step 1: Get familiar with the PCB

![Person examining the PCB](images/soldering/IMG_4557.jpeg)

Take a moment to look over the PCB. The center area is where the microcontroller mounts. The large circular pads are the prop LED pads; the gold traces connect everything together.

---

## Step 2: Inspect the PCB component side

![PCB front side showing component area](images/soldering/IMG_4558.jpeg)

The front side of the PCB has the small pre-soldered SMD components (capacitors, diodes, resistors) in the center. The two rows of through-holes flanking the center cutout are where the pin headers will go.

---

## Step 3: Gather your parts

![XIAO RP2040 with two pin header strips](images/soldering/IMG_4563.jpeg)

You need the XIAO RP2040 and two 7-pin header strips. The headers typically come packaged with the XIAO — break them to length if needed.

---

## Step 4: Insert the pin headers

![Inserting pin headers into PCB](images/soldering/IMG_4566.jpeg)

Insert the two pin header strips into the through-holes on the PCB (short pins down through the board, long pins up). **Tip:** resting the PCB on a roll of tape keeps it elevated so the pins don't get pushed back out while you work.

---

## Step 5: Pin headers seated

![PCB with pin headers inserted, center area visible](images/soldering/IMG_4568.jpeg)

With both header strips inserted, you can see the two rows of pins ready to accept the microcontroller. The pre-soldered SMD components (D1, D4, D5, R5, R6, U1, U2) are visible in the center — these are already done for you.

---

## Step 6: Place the microcontroller — correct orientation

![XIAO placed on PCB, USB port pointing toward tail](images/soldering/IMG_4570.jpeg)

Place the XIAO onto the pin headers. **The USB-C port must face toward the tail of the aircraft** (the bottom of the PCB as shown). This is critical — the pin mapping only works in this orientation.

---

## Step 7: Wrong orientation — don't do this

![XIAO placed backwards with red X](images/soldering/IMG_4571.jpeg)

**Do not** place the chip with the USB port pointing toward the nose. This is the wrong orientation — the pins will be reversed and the board won't work correctly.

---

## Step 8: Wrong orientation — don't do this either

![XIAO placed upside down with red X](images/soldering/IMG_4572.jpeg)

**Do not** place the chip upside down (component side facing down). The chip label and FCC logo should be visible from the top.

---

## Step 9: Turn on the soldering iron

![Soldering iron powered on at 300°C](images/soldering/IMG_4578.jpeg)

Power on your soldering iron and set it to **300°C (570°F)**. Allow it to fully heat up before starting — this usually takes 1–2 minutes.

---

## Step 10: Clean the tip

![Cleaning the iron tip in brass wool](images/soldering/IMG_4579.jpeg)

Before soldering, clean the iron tip by wiping it in the brass wool cleaner. A clean, shiny tip transfers heat much more effectively than a dirty or oxidized one. Re-tin the tip with a small amount of fresh solder after cleaning.

---

## Step 11: Double-check orientation before soldering

![Close-up of XIAO seated correctly, USB toward tail](images/soldering/IMG_4581.jpeg)

Before applying any solder, do a final orientation check. The USB-C port should be pointing toward the tail. Once you solder a pin it takes effort to undo, so confirm now.

---

## Step 12: Solder technique — one pin first

![Iron at pin/pad intersection with technique callouts](images/soldering/IMG_4584.jpeg)

Place the iron tip at the intersection of the pin and the copper pad — touching both at the same time. Feed solder into that junction (not onto the iron itself) until it flows smoothly, then remove the solder, then remove the iron. **Start with just one pin** so you can verify everything is flat before committing to all the others.

---

## Step 13: Where to apply solder

![Close-up of iron touching pin/pad junction](images/soldering/IMG_4593.jpeg)

Heat where the pin meets the copper pad. Add solder to the intersection of the pin and pad — not to the soldering iron. The solder should flow toward the heat and wick around the pin to form a cone shape.

---

## Step 14: What a good joint looks like

![Single bright smooth solder joint](images/soldering/IMG_4595.jpeg)

A good solder joint is **bright and smooth**, forming a neat cone around the pin. If it looks dull, lumpy, or balled up, it's a cold joint — reheat it until the solder flows and re-wets the pad.

---

## Step 15: Solder one full side

![All pins on one side soldered](images/soldering/IMG_4600.jpeg)

Once your first pin looks good and the chip is sitting flat, solder all the remaining pins on that side. Then do the other side. Check every joint — all should be bright and smooth.

---

## Step 16: Watch out for solder bridges (shorts)

![Close-up of solder bridge between pins with red X](images/soldering/IMG_4607.jpeg)

Watch for **solder bridges** — blobs of solder connecting two adjacent pins together. If you see one, reheat the joint until the solder melts, then drag the iron tip away to pull the excess solder off. You can also use solder wick to absorb it.

---

## Step 17: Tape the chip and flip the board

![PCB flipped over with yellow tape holding the chip](images/soldering/IMG_4609.jpeg)

Place a strip of tape over the chip to hold it in place, then carefully flip the board over. The pin tips will now be sticking up through the back of the PCB, ready to solder.

---

## Step 18: Back side — pins ready to solder

![Back of PCB with pin tips protruding through](images/soldering/IMG_4610.jpeg)

From the back you can see the two rows of pin tips coming through the board. These are what you'll solder to secure the connection from this side.

---

## Step 19: Back side — solder one pin first

![Back of PCB, one corner pin soldered](images/soldering/IMG_4612.jpeg)

Same technique as the front: start with just one corner pin. Solder it, then check that the chip is still sitting flat against the PCB before continuing.

---

## Step 20: Check flatness, then solder the opposite corner

![Back of PCB, opposite corner pin now soldered](images/soldering/IMG_4614.jpeg)

Once the first pin is done and the chip is confirmed flat, solder the diagonally opposite corner pin. These two anchors hold the chip securely in position while you finish all the remaining pins.

---

## Step 21: Watch out for cold joints

![Back pins with cold joints marked with red X](images/soldering/IMG_4616.jpeg)

**Cold joints** look dull, grainy, or like a ball sitting on the pad rather than flowing into it. They indicate the solder didn't reach proper temperature and may not make a good electrical connection. Re-melt any cold joints and let them re-flow properly.

---

## Step 22: Finish all back pins

![All back pins soldered, flux residue visible](images/soldering/IMG_4619.jpeg)

Solder all remaining pins on the back side. All joints should be smooth and bright. Don't worry about the amber/brown flux residue — that's normal and harmless, and you'll clean it off next.

---

## Step 23: Clean off the flux

![Board being cleaned with isopropyl alcohol and cotton swab](images/soldering/IMG_4620.jpeg)

Dip a cotton swab in isopropyl alcohol (IPA, 90%+) and scrub away the flux residue. The board will look much cleaner and it's easier to inspect your joints afterwards.

---

## Step 24: Trim the protruding pins

![Person trimming pins with flush cutters, wearing safety glasses](images/soldering/IMG_4624.jpeg)

Use flush cutters to trim the pin tips down close to the solder joints. **Always wear safety glasses for this step** — the clipped pin ends can fly off at speed and cause eye injury.

---

## Step 25: Done!

![Back of finished PCB with clean trimmed pins](images/soldering/IMG_4629.jpeg)

Congratulations — you're done! The back of the board should show two neat rows of short, trimmed, bright solder joints. Plug in your USB-C cable, upload the firmware, and enjoy the light show.

---

## Troubleshooting

**LEDs don't light up at all**
- Check the serial monitor (115200 baud) — the board should print startup info on boot
- Verify the USB port is toward the tail (correct orientation)
- Inspect all solder joints under good lighting — look for any that are dull or bridged

**Some LEDs are dim or wrong color**
- Usually a cold joint on one of the data pins — reflow any suspect joints

**Board won't enumerate as USB device**
- Check for a solder bridge on the pins nearest the USB connector
- Try holding the BOOT button while plugging in to enter bootloader mode

See [TUTORIAL.md](TUTORIAL.md) for full firmware setup instructions.
