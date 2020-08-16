#include "sys.h"
#include "bios.h"
#include <libdragon.h>
#include "libgbpak.h"
#include "errcodes.h"

/*
8MB ROM and 128KB RAM are the max supported by libgbpak
The N64 only has 4MB RAM, so creating a buffer much larger than 2MB won't work.
Need to stream the data out somewhere to support the subset of games that are >2MB.
*/
#define BUFFER_SIZE 1024*1024*2

extern cart gbcart;
extern u8 g_last_y;

/*
 Holds the last operation performed. Used to display relevant completion messages to the user.
 Set to NONE when the user cancels an operation.
*/
typedef enum
{
    NONE = 0,
    READ_RAM,
    READ_ROM,
    TRANSMIT,
    RECEIVE,
    WRITE_RAM,
} ops_t;

/*
 Updated to reflect the current buffer contents. Used to report the buffer state to the user and to 
 perform validity checking on operations.
*/
typedef enum
{
    EMPTY = 0,
    CART_RAM,
    CART_ROM,
    USB_RAM,
} contents_t;

u16 char_buff[G_SCREEN_W * G_SCREEN_H];

err_t res = ERR_NONE;
ops_t op = NONE;
contents_t contents = EMPTY;
/*used to limit displaying cartridge metadata only when init_gbpak is successful*/
uint8_t cartDataGood = FALSE;
unsigned sum = 0; //simple checksum
uint8_t buffer[BUFFER_SIZE] = {0};  

void gBlankCurrentLine()
{
    gSetXY(G_BORDER_X, g_last_y);
    gAppendString("                                    "); //36 characters
    gSetXY(G_BORDER_X, g_last_y);
}

void printMapper(void)
{
    switch(gbcart.mapper)
    {
        case GB_NORM:
            gAppendString("none");
            break;
        case GB_MBC1:
            gAppendString("MBC1");
            break;
        case GB_MBC2:
            gAppendString("MBC2");
            break;
        case GB_MMMO1:
            gAppendString("MMM01");
            break;
        case GB_MBC3:
            gAppendString("MBC3");
            break;
        case GB_MBC5:
            gAppendString("MBC5");
            break;
        case GB_CAMERA:
            gAppendString("(Camera)");
            break;
        case GB_TAMA5:
            gAppendString("TAMA5");
            break;
        case GB_HUC3:
            gAppendString("HUC3");
            break;
        case GB_HUC1:
            gAppendString("HUC1");
            break;
        default:
            gAppendString("unknown");
            break;
    }
}

void printCartData()
{
    if (!cartDataGood) return;
    
    gConsPrint("");
    gAppendString("Title: ");
    gAppendString(gbcart.title);
    gConsPrint("");
    gAppendString("Mapper: ");
    printMapper();
    gConsPrint("");
    gAppendString("ROM Size: ");
    gAppendHex32(gbcart.romsize);
    gConsPrint("");
    gAppendString("RAM Size: ");
    gAppendHex32(gbcart.ramsize);	
}

void printOperationResult()
{
    if (NONE == op || ERR_NONE != res) return;

    switch (op)
    {
        case READ_RAM:
            gConsPrint("SRM data read from cart.");
            break;
        case READ_ROM:
            gConsPrint("ROM data read from cart.");
            break;
        case TRANSMIT:
            gConsPrint("Data send complete.");
            break;
        case RECEIVE:
            gConsPrint("Data receive complete.");
            break;
        case WRITE_RAM:
            gConsPrint("SRM data written to cart.");
            break;
        default: //unreachable
            break;
    }
}

void printBufferContents()
{
    if (EMPTY != contents)
    {
        gConsPrint("Current data buffer contents: ");
        switch (contents)
        {
            case CART_RAM:
                gConsPrint("  Cart RAM data");
                break;
            case CART_ROM:
                gConsPrint("  Cart ROM data");
                break;
            case USB_RAM:
                gConsPrint("  RAM data from USB");
                break;
            default: //unreachable
                break;
        }
        //don't print checksum if rom is larger than our buffer size
        if (BUFFER_SIZE >= gbcart.romsize)
        {
            gConsPrint("");
            gAppendString("  Checksum: ");
            gAppendHex32(sum);
        }
    }    
}

void printError()
{
    switch(res)
	{
		case ERR_GB_INIT:
			gConsPrint("Error connecting to Transfer Pak.");
			return;
		case ERR_RAM_READ:
			gConsPrint("Error reading cart RAM.");
			return;
		case ERR_RAM_WRITE:
			gConsPrint("Error writing cart RAM.");
			return;
		case ERR_ROM_READ:
			gConsPrint("Error reading cart ROM.");
			return;
		case ERR_TRANSMIT:
			gConsPrint("USB transmission error.");
			return;
        case ERR_BUFFER_EMPTY:
            gConsPrint("Buffer is empty.");
            gConsPrint("nothing to send/write.");
            return;
        case ERR_BUFFER_CONTAINS_ROM:
            gConsPrint("Buffer currently contains ROM.");
            gConsPrint("Load RAM before writing.");
            break;
		default:
			break;
	}
}

void printMenu()
{
	gConsPrint("GB(C) Transfer Pak ROM/RAM Utility");
	gConsPrint("A - Read new cart (RAM)");
	gConsPrint("B - Read new cart (ROM)");
	gConsPrint("L - Transmit current buffer data");
	gConsPrint("R - Receive RAM data");
    gConsPrint("Start - Write RAM data to cart");
    gConsPrint("");
	
    printCartData();
    printBufferContents();
    printOperationResult();
    printError();
}

void simpleChecksum(int size)
{
    sum = 0;
    for(unsigned i = 0; i < size; i++)
    {
        sum += buffer[i];
    }    
}

void usbTransmit(unsigned dataLen)
{
    int retval = 0;
    
    gConsPrint("Sending data over USB...");
    gConsPrint("B to cancel");
    gConsPrint("");
    
    for(unsigned i = 0; i < dataLen; i+= BI_MIN_USB_SIZE)
    {                        
        if (0 == i % (dataLen / 32))
        {
            gBlankCurrentLine();
            gAppendString("  Sent ");
            gAppendHex32(i);
            gAppendString(" bytes");
            sysRepaint();
        }
        //allow user to cancel the send whenever it's sitting in a wait state
        while(!bi_usb_can_wr())
        {
            controller_scan();
            struct controller_data keys = get_keys_down();
            for( int j = 0; j < 4; j++ )
            {
                if( keys.c[j].B )
                {
                    op = NONE;
                    // goto MAIN_LOOP_TOP;
                    return;
                }
            }
        }                               
        retval = bi_usb_wr(buffer + i, BI_MIN_USB_SIZE); //is 16 bytes necessary or was that arbitrary?
        if (0 != retval )
        {
            res = ERR_TRANSMIT;
            return;
        }
    }    
}

int main(void) 
{    
	uint8_t startup = 0;
	int retval = 0;
	// uint8_t buffer[BUFFER_SIZE] = {0};  
	
    sysInit();
    gInit(char_buff);
    bi_init();	
	
	controller_init();
	
    cartDataGood = TRUE;
	retval = init_gbpak();
    if (retval) 
    {    
        res = ERR_GB_INIT;
        cartDataGood = FALSE;
    }    
    
    while (1) 
	{
MAIN_LOOP_TOP:
		gCleanScreen();
		printMenu();
		
		controller_scan();
		struct controller_data keys = get_keys_down();
        
		/* noticed buttons being tripped if checked too early */
		if (startup)
		{
			for( int i = 0; i < 4; i++ )
			{
				if( keys.c[i].A )
				{
                    op = READ_RAM;
                    res = ERR_NONE;
                    	
					retval = init_gbpak();
                    if (retval) 
                    {    
                        res = ERR_GB_INIT;
                        cartDataGood = FALSE;
                        break;
                    }
                    cartDataGood = TRUE;

                    gConsPrint("Reading RAM data from cart...");
                    sysRepaint();
					retval = copy_gbRam_toRAM(buffer);
                    if (retval)
                    {
                        res = ERR_RAM_READ;
                        contents = EMPTY;
                        break;
                    }
                    contents = CART_RAM;

                    simpleChecksum(gbcart.ramsize);
                    break;                    
				}
                
                if( keys.c[i].B )
				{
                    op = READ_ROM;
                    res = ERR_NONE;
                    		
					retval = init_gbpak();
                    if (retval) 
                    {    
                        res = ERR_GB_INIT;
                        cartDataGood = FALSE;
                        break;
                    }
                    cartDataGood = TRUE;

                    if (BUFFER_SIZE < gbcart.romsize)
                    {
                        
                        gConsPrint("ROM data too large to store.");
                        gConsPrint("Send ROM data over USB now?");
                        gConsPrint("A to proceed");
                        gConsPrint("B to cancel");
                        sysRepaint();
                        
                        while (1)
                        {
                            controller_scan();
                            struct controller_data keys = get_keys_down();
                            for( int i = 0; i < 4; i++ )
                            {
                                if( keys.c[i].A )
                                {
                                    //treat as empty. one chunk of ROM isn't very useful alone.
                                    contents = EMPTY;
                                    
                                    //read ROM in 2MB chunks
                                    for (uint32_t bankOffset = 0; bankOffset < gbcart.rombanks;)
                                    {
                                        op = NONE;
                                        gCleanScreen();
                                        printMenu();
                                        op = TRANSMIT; //lazy fix for our calling printMenu() at a weird time.
                                        
                                        gConsPrint("Reading ROM data from cart...");
                                        sysRepaint();
                                        retval = copy_gbRom_toRAM(buffer, &bankOffset, BUFFER_SIZE);
                                        if (retval)
                                        {
                                            res = ERR_ROM_READ;
                                            goto MAIN_LOOP_TOP;
                                        }
                                        
                                        /*the only ROM sizes above 2MB supported are multiples of 2MB.
                                        If there are other sizes the code will have to be updated
                                        or the final buffer sent will have to be post-processed*/
                                        usbTransmit(BUFFER_SIZE);
                                        if (NONE == op || ERR_NONE != res)
                                        {                                            
                                            goto MAIN_LOOP_TOP;
                                        }
                                    }
                                    goto MAIN_LOOP_TOP;
                                }
                                if( keys.c[i].B ) 
                                {
                                    op = NONE;
                                    goto MAIN_LOOP_TOP;
                                }
                            }
                        }                        
                    }
                    else 
                    {
                        gConsPrint("Reading ROM data from cart...");
                        sysRepaint();
                        retval = copy_gbRom_toRAM(buffer, NULL, 0);
                        if (retval)
                        {
                            res = ERR_ROM_READ;
                            contents = EMPTY;
                            break;
                        }
                        contents = CART_ROM;
                        simpleChecksum(gbcart.romsize);                        
                    }
                    
                    break;                    					
				}

				if( keys.c[i].start )
				{
                    op = WRITE_RAM;
                    res = ERR_NONE;
                    if (EMPTY == contents)
                    {
                        res = ERR_BUFFER_EMPTY;
                        break;
                    }
                    if (CART_ROM == contents)
                    {
                        res = ERR_BUFFER_CONTAINS_ROM;
                        break;
                    }                    

                    gConsPrint("WARNING");
                    gConsPrint("Overwrite game save data?");
                    gConsPrint("A to proceed");
                    gConsPrint("B to cancel");
                    sysRepaint();
                    while (1)
                    {
                        controller_scan();
                        struct controller_data keys = get_keys_down();
                        for( int i = 0; i < 4; i++ )
                        {
                            if( keys.c[i].A )
                            {
                                gConsPrint("Writing RAM data to cart...");
                                sysRepaint();
                                retval = copy_save_toGbRam(buffer);
                                if (retval)
                                {
                                    res = ERR_RAM_WRITE;
                                }
                                goto MAIN_LOOP_TOP;
                            }
                            if( keys.c[i].B ) 
                            {
                                op = NONE;
                                goto MAIN_LOOP_TOP;
                            }
                        }
                    }
                }

				if( keys.c[i].L )
				{
                    op = TRANSMIT;
                    res = ERR_NONE;
                    unsigned dataLen = 0;
                    
                    //what does the buffer currently contain?
                    if (CART_RAM == contents || USB_RAM == contents)
                    {
                        dataLen = gbcart.ramsize;
                    }
                    else if (CART_ROM == contents)
                    {
                        dataLen = gbcart.romsize;
                    }
                    else
                    {
                        res = ERR_BUFFER_EMPTY;
                        continue;
                    }
                    
                    usbTransmit(dataLen);
                    // gConsPrint("Sending data over USB...");
                    // gConsPrint("B to cancel");
                    // gConsPrint("");
                    
					// for(unsigned i = 0; i < dataLen; i+= BI_MIN_USB_SIZE)
					// {                        
                        // if (0 == i % 1024)
                        // {
                            // gBlankCurrentLine();
                            // gAppendString("  Sent ");
                            // gAppendHex32(i);
                            // gAppendString(" bytes");
                            // sysRepaint();
                        // }
                        // // allow user to cancel the send whenever it's sitting in a wait state
						// while(!bi_usb_can_wr())
                        // {
                            // controller_scan();
                            // struct controller_data keys = get_keys_down();
                            // for( int i = 0; i < 4; i++ )
                            // {
                                // if( keys.c[i].B )
                                // {
                                    // op = NONE;
                                    // goto MAIN_LOOP_TOP;
                                // }
                            // }
                        // }                               
						// retval = bi_usb_wr(buffer + i, BI_MIN_USB_SIZE); //is 16 bytes necessary or was that arbitrary?
						// if (0 != retval )
						// {
                            // res = ERR_TRANSMIT;
							// break;
						// }
					// }
                    break;                    
				}

				if( keys.c[i].R )
				{
                    res = ERR_NONE;
                    op = RECEIVE;
                    
                    gConsPrint("Receiving data over USB...");
                    gConsPrint("");
					for(unsigned i = 0; i < gbcart.ramsize; i+= BI_MIN_USB_SIZE)
					{
                        if (0 == i % 1024)
                        {                        
                            gBlankCurrentLine();
                            gAppendString("  Received ");
                            gAppendHex32(i);
                            gAppendString(" bytes");
                            sysRepaint();
                        }
                        
						while(!bi_usb_can_rd())
                        {
                            controller_scan();
                            struct controller_data keys = get_keys_down();
                            for( int i = 0; i < 4; i++ )
                            {
                                if( keys.c[i].B )
                                {
                                    op = NONE;
                                    contents = EMPTY;
                                    goto MAIN_LOOP_TOP;
                                }
                            }
                        }                               
						retval = bi_usb_rd(buffer + i, BI_MIN_USB_SIZE);
						if (0 != retval )
						{
                            res = ERR_TRANSMIT;
                            contents = EMPTY;
							break;
						}
					} 
                    contents = USB_RAM; //doesn't make sense to receive ROM
                    break;                    
				}
			}
		}		
		
        sysRepaint();
		startup = 1;
    }
	
	return 0;
}