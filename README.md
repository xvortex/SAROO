
### SAROO is a HDLoader for SEGA Saturn.

SAROO is a Sega Saturn optical drive emulator. SAROO is inserted into the card slot and realizes the CDBLOCK function of the original motherboard, loading games from the SD card and running them.
SAROO also provides 1MB/4MB accelerator card function.

--------
### Some pictures

<img src="doc/saroo_v12_top.jpg" width=48%/>  <img src="doc/saroo_v12_bot.jpg" width=48%/>
<img src="doc/saroo_scr1.png" width=48%/>  <img src="doc/saroo_scr2.png" width=48%/>
<img src="doc/saroo_dev1.png"/>
<img src="doc/saroo_devhw.jpg"/>


--------
### Development History

#### V1.0
The original SAROO just added a usbhost interface to the common usbdevcart. It is necessary to crack the main program of the game and convert the operation of CDBLOCK into the operation of U disk.
This method needs to be modified for each game and is not universal. There are also big problems with performance and stability. Only a few games have been launched this way.
(V1.0 related files are not included in this project)

#### V1.1
The new version has a completely new design. Adopt FPGA+MCU method. FPGA (EP4CE6) is used to implement the hardware interface of CDBLOCK, and MCU (STM32F103) runs firmware to process various CDBLOCK commands.
This version has basically achieved the intended purpose, and some games can almost run. But there is also a fatal problem: random data errors. Various mosaics will appear when playing the title animation.
and eventually died. This problem is difficult to debug and locate. This resulted in the project being stalled for a long time.

#### V1.2
Version 1.2 is a bugfix and performance improvement of version 1.1, using a higher-performance MCU: STM32H750. It's high enough frequency (400MHz) and has enough SRAM inside to accommodate a full CDC cache.
The FPGA has also been restructured internally, abandoning the qsys system and using its own SDRAM and bus structure. This version lives up to expectations and is already in near-perfect condition.
At the same time, by back-porting the FPGA and MCU firmware to V1.1 hardware, V1.1 has basically reached the performance of V1.2.

--------
### Current status

Dozens of games tested worked perfectly.
The 1MB/4MB accelerator card function can be used normally.
SD card supports FAT32/ExFAT file system.
Supports image files in cue/bin format. Single bin or multiple bins.
Some games will get stuck in the loading/title animation interface.
Some games will get stuck while playing.

--------
### Hardware and Firmware

The schematic diagram and PCB are produced using AltiumDesigner 14.
Version V1.1 requires flying cables to work properly. This version should no longer be used.
The V1.2 version still requires an additional pull-up resistor to use the FPGA's AS configuration mode.

FPGA is developed using Quartus 14.0.

Firm_Saturn is compiled using the SH-ELF compiler that comes with SaturnOrbit.

Firm_v11 is compiled using MDK4.
Firm_V12 is compiled using MDK5.

--------
### SD card file placement

<pre>
/ramimage.bin       ;Saturn firmware program;
/SAROO/saroocfg.txt ;configuration file;
/SAROO/ISO/         ;Stores game images. One game per directory. The directory name will be displayed in the menu;
/SAROO/BIN/         ;Stores bins. One bin per directory. The directory name will be displayed in the menu;
/SAROO/update/      ;Stores firmware upgrades;
                    ;  FPGA: SSMaster.rbf
                    ;  MCU : mcuboot.bin
</pre>


--------
一Some features under development: [SAROO tech info](doc/SAROO技术点滴.txt)
