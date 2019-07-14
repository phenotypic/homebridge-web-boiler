# OpenTherm PCB

In this folder, you can find the raw `.fzz` file, the exported gerber files and a bill of materials.

The gerber files (`Gerber` folder) are ready to be sent off to a PCB fabrication company and the bill of materials (`BOM.md`) lists all the components you will need to populate the board.

## Assembly

- The PCB is completely through-hole and is therefore straight forward to assemble. The silkscreen layer indicates which components go where as well as which direction they should face (if that matters for the component). 

- **Please note:** there are **5** places on the board where two leads from different components must go through the **same hole**. Whilst this is slightly inconvenient, it was the only way to get the PCB to such a small size. To help, the silkscreen layer includes small `!` marks next to holes where this is required.

- The PCB has three extra pins available (labelled `SENSOR`) where you can optionally solder in the DHT11 module, providing it with `VCC`, `GND` (marked with `+` and `-`) and a data out pin which corresponds to a pin on the header block. **This is completely optional** and the PCB will function normally even without anything soldered in these holes. 

## Usage

- OpenTherm polarity does not matter, so the two OpenTherm wires can go into either of the screw terminals

- Pins on the header are labelled where the optocouplers will go. From top to bottom (with the screw terminal on the left), the pins are: `GND`, `TMP`, `OUT`, `VCC` and `IN`. As mentioned above, the `TMP` provides temperature date if you have soldered in your own temperature sensor. If you have not, that is fine, you do not have to use this pin.

- The PCB **does not** power your device, you must do that separately

## Photos

![Top](https://i.ibb.co/gPgSYPL/Top.jpg)

![Front](https://i.ibb.co/gjwf7yr/Front.jpg)

## Notes

- The dimensions are `39` x `18.5` mm

- The board is single-sided (copper traces only on one side)

- The PCB visible in the pictures above is the first generation of the board and lacks some silkscreen markings which are mentioned above (which **are** present in the version provided here)

- Where there are two resistor values marked `1KΩ` and `510Ω` respectively (they go through same hole), you can use any two resistor values you want as long as they add up to around `1.5kΩ`
