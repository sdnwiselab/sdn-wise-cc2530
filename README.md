SDN-WISE-CC2530
====================================

### SDN-WISE for EMB-Z2530PA Motes

Based on [TIMAC](http://www.ti.com/tool/TIMAC)

### Build 
* Download TIMAC from this [link](http://www.ti.com/tool/TIMAC). 
* Copy the sdn-wise folder inside: 

`C:\Texas Instruments\TIMAC 1.5.x\Components`

* Then open with [IAR](https://www.iar.com/iar-embedded-workbench/#!?architecture=8051) the project in:

`C:\Texas Instruments\TIMAC 1.5.x\Projects\mac\Sample\cc2530\IAR Project\`

* Finally, add the sdn-wise forder to the build path and use `sdn-wise/common/sdn_wise.c` as main file for your firmware.

### Dependencies

Tested with TIMAC 1.5.0 and IAR Embedded Workbench 6.5

### Hardware Support
This implementation works only with EMB-Z2530PA Motes. 
Please check `sdn-wise-contiki` [here](https://github.com/sdnwiselab/sdn-wise-contiki) if you have a different model of mote.