/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/lg4573.h>
#include <mach/gpio.h>
#include <mach/gpio-wave.h>
#include <linux/delay.h>

const unsigned short LG4573_SEQ_SETTING_TYPE_0[] = {
	SLEEPMSEC, 120,
	0xC1, // Stand-by off
	0x100,

	0x11,   // Exit Sleep Mode

	0x36,
	0x100,

	0x3A,
	0x170,  //24 bit interface

	0xB1,  //RGB interface setting
	0x110,  //VSYNC+HSYNC Rising edge HSYNC - active high VSYNC-active high DE-active high
	0x111,  // HBP -- 11, 10, 11, 13, 18, 1A, 14 Inc : L, Dec : R
	0x102,  // VBP -  3, 5, 7, 1C, 10, 0C  Inc : U, Dec : D


	0xB2,
	0x110,  //HYDYS panel, 480pexel
	0x1C8,  //800 pixels

	0xB3,
	0x100, // 0x01 : 1-Dot Inversion, 0x00 : Column Inversion

	0xB4,
	0x104,

	0xB5,
	0x105, // 5, 10, FF
	0x110,
	0x110,
	0x100,
	0x100,

	0xB6,
	0x101,  // overlaping
	0x101,
	0x102,
	0x140,
	0x102,
	0x100,

//Power controll
	0xC0,
	0x100,
	0x11F,

	0xC2,
	0x100,

	0xC3,
	0x103,
	0x104,
	0x103,
	0x103,
	0x103,

	0xC4,
	0x102,
	0x123,
	0x10E,
	0x10E,
	0x102,
	0x17A,

	0xC5,
	0x174,

	0xC6,
	0x124,
	0x160,
	0x100,

//Gamma controll
	0xD0, // Gama Set_1
	0x142,
	0x113,
	0x141,
	0x116,
	0x107,
	0x103,
	0x161,
	0x116,
	0x103,

	0xD1,  	// Gama Set_1
	0x142,
	0x113,
	0x141,
	0x116,
	0x107,
	0x103,
	0x161,
	0x116,
	0x103,

	0xD2,  	// Gama Set_1
	0x142,
	0x113,
	0x141,
	0x116,
	0x107,
	0x103,
	0x161,
	0x116,
	0x103,

	0xD3,  	// Gama Set_1
	0x142,
	0x113,
	0x141,
	0x116,
	0x107,
	0x103,
	0x161,
	0x116,
	0x103,

	0xD4,  	// Gama Set_1
	0x142,
	0x113,
	0x141,
	0x116,
	0x107,
	0x103,
	0x161,
	0x116,
	0x103,

	0xD5,  	// Gama Set_1
	0x142,
	0x113,
	0x141,
	0x116,
	0x107,
	0x103,
	0x161,
	0x116,
	0x103,

	SLEEPMSEC, 100,

	0x29, //Disp ON

	ENDDEF, 0x0000
};

const unsigned short LG4573_SEQ_SETTING_TYPE_2[] = {
	0xF1,
	0x15A,
	0x15A,

	SLEEPMSEC, 10,
	0xB7,
	0x100,
	0x111,
	0x111,

	0xB8,
	0x10C,
	0x110,

	0xB9,
	0x100,
	0x106,

	0xBB,
	0x100,

	0xC0,
	0x180,
	0x110,

	0xC1,
	0x108,

	0xEE,
	0x112,

	0xF2,
	0x100,
	0x130,
	0x188,
	0x188,
	0x157,
	0x157,
	0x100,
	0x141,
	0x100,

	0xF3,
	0x100,
	0x110,
	0x125,
	0x101,
	0x12D,
	0x12D,
	0x124,
	0x12D,
	0x110,
	0x110,
	0x10A,
	0x137,

	0xF4,
	0x100,
	0x120,
	0x100,
	0x1AF,
	0x164,
	0x100,
	0x1AF,
	0x164,
	0x101,
	0x101,

	0xF5,
	0x100,
	0x100,
	0x157,
	0x111,
	0x105,
	0x101,
	0x101,
	0x101,
	0x102,
	0x102,

	0xF6,
	0x1A1,
	0x100,
	0x1C0,
	0x100,
	0x100,

	0xF7,
	0x104,
	0x16E,
	0x100,
	0x112,
	0x103,
	0x10D,
	0x10A,
	0x116,
	0x105,
	0x104,
	0x10E,
	0x104,
	0x104,
	0x100,
	0x196,
	0x101,
	0x118,

	0xF8,
	0x104,
	0x16E,
	0x100,
	0x100,
	0x103,
	0x10D,
	0x10A,
	0x116,
	0x105,
	0x104,
	0x10E,
	0x104,
	0x104,
	0x100,
	0x196,
	0x101,
	0x118,

	0xF9,
	0x101,
	0x102,
	0x105,
	0x104,
	0x10A,
	0x10B,
	0x100,
	0x106,
	0x111,
	0x110,
	0x10F,
	0x100,
	0x100,
	0x100,
	0x100,
	0x100,
	0x100,
	0x100,
	0x100,
	0x1CF,
	0x1FF,
	0x1F0,

	0xFA,
	0x100,
	0x12C,
	0x12B,
	0x125,
	0x139,
	0x137,
	0x134,
	0x126,
	0x120,
	0x134,
	0x11F,
	0x11D,
	0x11D,
	0x10E,
	0x104,
	0x11C,
	0x12E,
	0x12C,
	0x124,
	0x136,
	0x136,
	0x137,
	0x128,
	0x123,
	0x138,
	0x125,
	0x125,
	0x129,
	0x121,
	0x11D,
	0x11D,
	0x12E,
	0x12D,
	0x125,
	0x139,
	0x137,
	0x137,
	0x129,
	0x124,
	0x138,
	0x125,
	0x124,
	0x128,
	0x121,
	0x11E,

	0xFB,
	0x100,
	0x13F,
	0x13A,
	0x12F,
	0x140,
	0x13E,
	0x13A,
	0x123,
	0x116,
	0x12D,
	0x11D,
	0x119,
	0x116,
	0x108,
	0x101,
	0x100,
	0x123,
	0x121,
	0x11D,
	0x135,
	0x138,
	0x136,
	0x121,
	0x116,
	0x12D,
	0x11D,
	0x11D,
	0x11B,
	0x10C,
	0x103,
	0x100,
	0x122,
	0x121,
	0x11D,
	0x135,
	0x139,
	0x136,
	0x121,
	0x114,
	0x12C,
	0x11C,
	0x11B,
	0x119,
	0x10A,
	0x102,

	0xF1,
	0x1A5,
	0x1A5,

	0x2A,
	0x100,
	0x100,
	0x101,
	0x1DF,

	0x2B,
	0x100,
	0x100,
	0x103,
	0x11F,

	0x11,
	SLEEPMSEC, 5,

	0x3A,
	0x177,

	SLEEPMSEC, 100,
	0x29,
	ENDDEF, 0x0000
};

const unsigned short LG4573_SEQ_SETTING_TYPE_3[] = {
	SLEEPMSEC, 120,
	0xC1,	 // Stand-by off
	0x100,

	0x11,   // Exit Sleep Mode

	0x36,
	0x100,

	0x3A,
	0x170,  //24 bit interface

	0xB1,  //RGB interface setting
	0x110,  //VSYNC+HSYNC Rising edge HSYNC - active high VSYNC-active high DE-active high
	0x111,  // HBP -- 11, 10, 11, 13, 18, 1A, 14 Inc : L, Dec : R
	0x102,  // VBP -  3, 5, 7, 1C, 10, 0C  Inc : U, Dec : D


	0xB2,
	0x110,  //HYDYS panel, 480pexel
	0x1C8,  //800 pixels

	0xB3,
	0x100, // 0x01 : 1-Dot Inversion, 0x00 : Column Inversion

	0xB4,
	0x104,

	0xB5,
	0x105, // 5, 10, FF
	0x110,
	0x110,
	0x100,
	0x100,

	0xB6,
	0x101,  // overlaping
	0x101,
	0x102,
	0x140,
	0x102,
	0x100,

//Power controll
	0xC0,
	0x100,
	0x11F,

	0xC2,
	0x100,

	0xC3,	//Second Version LG4573
	0x103,
	0x104,
	0x105,	//0x103,
	0x106,	//0x103,
	0x101,	//0x103,

	0xC4,
	0x102,
	0x123,
	0x116,	//0x10E,
	0x116,	//0x10E,
	0x102,
	0x17A,

	0xC5,
	0x177,	//0x174,

	0xC6,
	0x124,
	0x160,
	0x100,

//Gamma controll
	0xD0,  	// Gama Set_1
	0x100,	//0x142,
	0x101,	//0x113,
	0x166,	//0x141,
	0x126,	//0x116,
	0x100,	//0x107,
	0x100,	//0x103,
	0x166,	//0x161,
	0x131,	//0x116,
	0x103,	//0x103,

	0xD1,  // Gama Set_2
	0x100,	//0x142,
	0x101,	//0x113,
	0x166,	//0x141,
	0x126,	//0x116,
	0x100,	//0x107,
	0x100,	//0x103,
	0x166,	//0x161,
	0x131,	//0x116,
	0x103,	//0x103,

	0xD2,  // Gama Set_1
	0x100,	//0x142,
	0x101,	//0x113,
	0x166,	//0x141,
	0x126,	//0x116,
	0x100,	//0x107,
	0x100,	//0x103,
	0x166,	//0x161,
	0x131,	//0x116,
	0x103,	//0x103,

	0xD3,  // Gama Set_2
	0x100,	//0x142,
	0x101,	//0x113,
	0x166,	//0x141,
	0x126,	//0x116,
	0x100,	//0x107,
	0x100,	//0x103,
	0x166,	//0x161,
	0x131,	//0x116,
	0x103,	//0x103,

	0xD4,  // Gama Set_1
	0x100,	//0x142,
	0x101,	//0x113,
	0x166,	//0x141,
	0x126,	//0x116,
	0x100,	//0x107,
	0x100,	//0x103,
	0x166,	//0x161,
	0x131,	//0x116,
	0x103,	//0x103,

	0xD5,  // Gama Set_2
	0x100,	//0x142,
	0x101,	//0x113,
	0x166,	//0x141,
	0x126,	//0x116,
	0x100,	//0x107,
	0x100,	//0x103,
	0x166,	//0x161,
	0x131,	//0x116,
	0x103,	//0x103,

	SLEEPMSEC, 100,

	0x29, //Disp ON

	ENDDEF, 0x0000
};

const unsigned short LG4573_SEQ_SLEEP_OFF[] = {
	0x11,
	SLEEPMSEC, 120,
	//0x29,
	SLEEPMSEC, 100,
	ENDDEF, 0x0000
};

const unsigned short LG4573_SEQ_SLEEP_ON[] = {
	0x10,
	SLEEPMSEC, 120,
	//0x28,
	SLEEPMSEC, 100,
	ENDDEF, 0x0000
};

int get_lcdtype(void)
{
	extern unsigned int HWREV;
	s3c_gpio_cfgpin(GPIO_OLED_ID, S3C_GPIO_INPUT);
	s3c_gpio_cfgpin(GPIO_DIC_ID, S3C_GPIO_INPUT);
	if(HWREV == 0x5)
		return (gpio_get_value(GPIO_DIC_ID) ? 3 : 0);
	if(HWREV == 0x6)
		return (gpio_get_value(GPIO_DIC_ID) << 1) | gpio_get_value(GPIO_OLED_ID);
	return 0;
}

struct s5p_lg4573_panel_data wave_lg4573_panel_data = {
	.seq_settings_type0 = LG4573_SEQ_SETTING_TYPE_0,
	/* TODO fix type1 table */
	.seq_settings_type1 = LG4573_SEQ_SETTING_TYPE_2,
	.seq_settings_type2 = LG4573_SEQ_SETTING_TYPE_2,
	.seq_settings_type3 = LG4573_SEQ_SETTING_TYPE_3,
	.seq_standby_on = LG4573_SEQ_SLEEP_ON,
	.seq_standby_off = LG4573_SEQ_SLEEP_OFF,

	.get_lcdtype = &get_lcdtype,
};
