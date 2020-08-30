# Subaru-ABS-converter
This converts modern hall sensor inputs to older variable reluctance outputs using a Maxim 9921 IC to read 2-wire hall sensors and a Teensy 3.2 for a microcontroller. 
Use this device if you are installing new hubs that use hall wheel speed sensors on old cars with variable reluctance ABS tone-rings.

The output stage is a 2-bit digital output fed to an audio transformer (1:1 600ohm:600ohm). 
The transformer removes the DC bias and presents an inductive source to the OEM ABS inputs (to circumvent open- or short-circuit error detection).

Set the input wheel tooth count to the OEM VR wheel tooth count. 

Set the output wheel tooth count to 2x the number of magnetic poles (count poles using magentic film, see image of subaru encoder). 
The Max9921 counts both the rising and falling edge of each magnetic pole, hence the multiplier.

This also includes a generic digital output labeled "VSS". This allows the device to present a VSS signal calculated as the average of both inputs.
The VSS output can be used to provide a VSS signal in the case of a new transmission without a built-in VSS sensor.
The VSS output, as installed in my car, just drives an LED and is used for status reporting. This needs to be modified if you want a proper VSS output.

There is an additional op-amp stage, but this has been left un-routed due to pure laziness. 

This setup produces and unknown amount of jitter, but it should be relatively small. 

Any error reported by the Max9921 will disable the device until reboot.

Use at your own risk. 
