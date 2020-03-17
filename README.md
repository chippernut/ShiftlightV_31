# ShiftlightV_31🏎
Open Source Sequential Shift light V3.1
Improvements from V3 include
+ Upgraded, Faster LEDs (APA102C Dotstar) replace old WS2812B
+ New graphic OLED display
+ Vastly improved RPM circuitry, power regulation, and protection
+ New Animations (out-to-center now included)
+ Optimized code and calculations
+ Auto-shut off 
+ USB output and dim icons 

## Future Road Map🏁
### SOFTWARE
- [ ]  If space allows, add knight rider "kitt" animation to be activated when 0 RPM detected. Menu option to turn on/off, set color, speed, etc. 
- [ ]  Add "profiles" for settings (ie, normal driving, track, drifting, drag, etc)
- [ ]  in Debug mode, clean all other serial.print data so it's just RPM output in all instances 
- [ ]  Add more brightness resolution, or make existing scale more dramatic
- [ ]  Add a run-time "hour meter" option in the menu. Test to see if an RTC chip is needed. 
- [ ]  Add optional input for another warning light (ie, oil pressure, temperature, etc). User defined in menu
- [ ]  Add (or make option) number of color segments. (3 colors to 4 colors). 
- [ ]  Add Peak-Hold functionality for RPM

### HARDWARE
- [ ]  Test the pads on the large electrolytic cap. May need to adjust trace on PCB
- [ ]  Investigate intermittent floating input on dimmer circuit, may cause blinking while dimmer off