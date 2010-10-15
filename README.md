iDroid Project openiBoot
---------------------------------------------------
	Copyright (C) 2008 David Wang (planetbeing).

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

1. Compiling
---------------------------------------------------
**Build an ARM toolchain (this only needs to be done once - for subsequent builds this step can be omitted unless there has been a toolchain update):**

`sudo toolchain/build-toolchain.sh make`
	
Wait, for a long time, as the toolchain is compiled.

**Compile OpeniBoot:**

To run openiboot from the recovery mode (a.k.a iboot), you’ll need to create an img3 image.
You will need a system capable of running x86 Linux binaries (Build requires libusb, libreadline and pthread). If compiling on 64-bit linux, you may need libc6-dev-i386 and type export ARCH=x86_64 before you proceed with the next steps, All community builds are run on an Ubuntu 10.04 Lucid Lynx system.

Change into the openiboot subfolder

**For iPod touch, run:**
`PLATFORM=IPOD make openiboot.img3`

**For iPhone 2G, run:**
`PLATFORM=IPHONE make openiboot.img3`

**For iPhone 3G, run:**
`PLATFORM=3G make openiboot.img3`

**Compile all in tools/client/:**
`cd tools/client/linux && make all && cd ..`

- The linux client will also build on OSX with the correct libusb version installed.

2. Installing
---------------------------------------------------
**Copy openiboot.img3 into tools/client/linux**

**Put your phone into recovery mode**: hold home whilst powering on until you see the iTunes logo.

**Run (From the root of the openiboot tree):** 
`sudo tools/client/linux/loadibec openiboot/openiboot.img3`

You should now see openiBoot on your phone, use the volume buttons to scroll to the console icon, then press home

**Run the client:**
`sudo client/linux/oibc`

*NOTE: If you are on a mac, you will most likely need to do this as soon as you have selected the console - Linux does not manifest this issue.*

You should now see the same output on your computer, as is on your phone's screen.

**Type**: 
`install` and press return

This will take a while, your NOR will be backed up during this process, and can be found in the current directory as norbackup.dump but will flash openiBoot to your phone.

3. Installing multitouch firmware for openiBoot (This is optional)
-------------------------------------------------------------------
A. iPhone 3G user (Convert zephyr2.bin):

 *  Copy your zephyr2.bin to openiboot/mk8900image
 *  Go to "openiboot/mk8900image"
 *  From "openiboot/mk8900image" execute this command (please adjust path to your firmware location): 

	>mk8900image zephyr2.bin zephyr2.img3 template-3g.img3

 *  Using your favorite hex editor, edit "zephyr2.img3"
     change data at offset 0x10 to 0x13 from *cebi* to *2ztm* and save it.

   2. Installing zephyr2.img3
*   Run oibc executable
*   From oibc prompt send "zephyr2.img3" to your iphone (default address is 0x09000000):

	>!zephyr2.img3

	It will return this: file received (XXXXX bytes).

*   Install the firmware from the address and with the size the upload command returned:

    >multitouch_fw_install 0x09000000 XXXXX


B. iPhone 2G user (Convert zephyr_aspeed.bin and zephyr_main.bin):

*	Copy your zephyr bins (zephyr_aspeed.bin and zephyr_main.bin) to openiboot/mk8900image
*   Go to "openiboot/mk8900image"
*   From "openiboot/mk8900image" execute this command (please adjust path to your firmware location): 

	>mk8900image zephyr_aspeed.bin to zephyr_aspeed.img3 template.img3

*   Using your favorite hex editor, edit "zephyr_aspeed.img3"
     change data at offset 0x10 to 0x13 from *cebi* to *aztm* and save it.

*   From "openiboot/mk8900image" execute this command (please adjust path to your firmware location): 

	>mk8900image zephyr_main.bin to zephyr_main.img3 template.img3

*   Using your favorite hex editor, edit "zephyr_main.img3"
     change data at offset 0x10 to 0x13 from *cebi* to *mztm* and save it.

   2. Installing firmwares
*   Run oibc executable
*   From oibc prompt send "zephyr_aspeed.img3" to your iphone (default address is 0x09000000):

	>!zephyr_aspeed.img3

     It will return this: file received (XXXXX bytes).
*   From oibc prompt send "zephyr_main.img3" to your iphone, in this case we will copy to memory location next to zephyr_aspeed.img:

	>!zephyr_main.img3@0x09000000+XXXXX

     It will return this: file received (YYYYY bytes).

*   Install the firmware from the address and with the size the upload command returned:

	>multitouch_fw_install 0x09000000 XXXXX 0x09000000+XXXX YYYYY


4. Reporting issues/requesting features
--------------------------------------------------
The iDroid Project has an issue tracking system set up to handle bug reports & feature requests, please see [http://dev.idroidproject.org/projects/openiboot][1] for more information.

For anything else, we can be found lurking in #idroid-dev on irc.osx86.hu

  [1]: http://dev.idroidproject.org/projects/openiboot