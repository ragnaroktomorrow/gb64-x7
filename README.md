# gb64-x7
Everdrive-64 x7 Transfer Pak Utilities

This is a pair of tools that allow you to read/write a gb(c) cart via the N64 Transfer Pak, the Everdrive-64 x7 and a serial connection over USB.
It consists of a PC side app (just some serial IO) and an N64 side app. It's based on [libdragon](https://github.com/DragonMinded/libdragon) and [libgbpak](https://github.com/saturnu/libgbpak). 

## Usage

1. Set up your N64 with the Everdrive-64 x7, the Transfer Pak and your target cart (Note: you should be able to swap games while using the tool but the Transfer Pak needs to remain connected). 

2. Connect the Everdrive to your PC with a usb cable and power on the console.

3. Download usb64 from [here](http://krikzz.com/pub/support/everdrive-64/x-series/dev/usb64.exe) and use it to load gb64-x7 onto the everdrive:  
   *(Note the name of the COM port connection used to communicate with the everdrive)*
```
usb64.exe -rom=gb64-x7.v64 -start
```    

4. The gb64-x7 menu should display on the screen and cart data should also appear if a cart is in the Transfer Pak.

5. Use the relevant menu buttons on the N64 to read the RAM or ROM from the cart into memory
   *Note for carts with ROM sizes above 2MB, the read and transfer will happen as one step due to the N64's limited memory.*
   
6. Open a command prompt on your PC and navigate to the directory containing gb64x7.exe

7. Using the port name noted from your previous use of usb64 and the RAM/ROM size displayed on the screen for this cart, set the application up to receive data:
``` 
 gb64x7.exe -port=<COM port observed when running usb64> -file=testsave.srm -recv=8000
```

8. The application should display that it opened the port and is waiting to receive data
   *Note: You need to trigger the PC side before initiating the transfer on the N64*
   
9. Use the relevant menu buttons on the N64 to initiate the transfer to the PC.
   *Sending is the same as receiving, except you specify the -send flag instead of -recv and don't provide a size.*
   
## Troubleshooting
 
- Try not to move/jostle the controller while using this. The connections can be finicky and disturbing them can cause issues when reading/writing the cart.
- If there are connection errors or the Title metadata doesn't look right, first try reinserting the cartridge and performing another read. If that doesn't work try starting over and reconnecting everything with the power off.
- Please backup and test your saves before writing to the cart. There's no guarantee something won't go wrong and your save will get corrupted. As far as I know, reading should be safe.
- Unfortunately I don't own any games larger than 2MB, so YMMV when reading ROM data in those cases (I implemented that part blind (: ).

## Building

- See the [Releases](https://github.com/ragnaroktomorrow/gb64-x7/releases/tag/v1.0.0) for prebuilt binaries
- N64 side
  - Set up a mips64 toolchain for building [libdragon](https://github.com/DragonMinded/libdragon)
  - Add gb64-x7_N64 to the examples directory
  - Add font64.bin, incs.s, and the bios, gfx and sys source and header files to the gb64-x7_N64 directory. These files can be found [at krikzz site](http://krikzz.com/pub/support/everdrive-64/x-series/dev/usbio-sample.zip)  
  - run make  
- PC side
  - Open the solution file in Visual Studio and build the project
