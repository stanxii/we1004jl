/* ----------------------------------------------------------------------------
 *         ATMEL Microcontroller Software Support  -  ROUSSET  -
 * ----------------------------------------------------------------------------
 * Copyright (c) 2006, Atmel Corporation

 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the disclaiimer below.
 *
 * - Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the disclaimer below in the documentation and/or
 * other materials provided with the distribution.
 *
 * Atmel's name may not be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * DISCLAIMER: THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * ----------------------------------------------------------------------------
 * File Name           : at91sam9263ek.c
 * Object              :
 * Creation            : NLe Aug 8th 2006
 *-----------------------------------------------------------------------------
 */
#include "../../include/part.h"
#include "../../include/gpio.h"
#include "../../include/pmc.h"
#include "../../include/debug.h"
#include "../../include/sdramc.h"
#include "../../include/main.h"
#ifdef CFG_NANDFLASH
#include "../../include/nandflash.h"
#endif
#ifdef CFG_DATAFLASH
#include "../../include/dataflash.h"
#endif

static inline unsigned int get_cp15(void)
{
	unsigned int value;
	__asm__("mrc p15, 0, %0, c1, c0, 0" : "=r" (value));
	return value;
}

static inline void set_cp15(unsigned int value)
{
	__asm__("mcr p15, 0, %0, c1, c0, 0" : : "r" (value));
}


#ifdef CFG_HW_INIT
/*---------------------------------------------------------------------------- */
/* \fn    hw_init							       */
/* \brief This function performs very low level HW initialization	       */
/* This function is invoked as soon as possible during the c_startup	       */
/* The bss segment must be initialized					       */
/*---------------------------------------------------------------------------- */
void hw_init(void)
{
	/* Configure PIOs */
	const struct pio_desc hw_pio[] = {
#ifdef CFG_DEBUG
		{"RXD", AT91C_PIN_PC(30), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"TXD", AT91C_PIN_PC(31), 0, PIO_DEFAULT, PIO_PERIPH_A},
#endif
		{(char *) 0, 0, 0, PIO_DEFAULT, PIO_PERIPH_A},
	};

	/* Disable watchdog */
	writel(AT91C_WDTC_WDDIS, AT91C_BASE_WDTC + WDTC_WDMR);

	/* At this stage the main oscillator is supposed to be enabled
	 * PCK = MCK = MOSC */

	/* Configure PLLA = MOSC * (PLL_MULA + 1) / PLL_DIVA */
	pmc_cfg_plla(PLLA_SETTINGS, PLL_LOCK_TIMEOUT);

	/* Switch MCK on PLLA output PCK = PLLA = 2 * MCK */
	pmc_cfg_mck(MCKR_SETTINGS, PLL_LOCK_TIMEOUT);

	/* Configure PLLB */
	pmc_cfg_pllb(PLLB_SETTINGS, PLL_LOCK_TIMEOUT);

	/* Configure the PIO controller to output PCK0 */
	pio_setup(hw_pio);

	/* Configure the EBI0 Slave Slot Cycle to 64 */
	writel( (readl((AT91C_BASE_MATRIX + MATRIX_SCFG4)) & ~0xFF) | 0x40, (AT91C_BASE_MATRIX + MATRIX_SCFG4));

#ifdef CFG_DEBUG
	/* Enable Debug messages on the DBGU */
	dbg_init(BAUDRATE(MASTER_CLOCK, 115200));
	dbg_print("Start AT91Bootstrap...\n\r");
#endif /* CFG_DEBUG */

#ifdef CFG_SDRAM
	/* Initialize the matrix */
	/* VDDIOMSEL = 1 -> Memories are 3.3V powered */
	writel(readl(AT91C_BASE_CCFG + CCFG_EBI0CSA) | (1 << 16) | AT91C_EBI_CS1A_SDRAMC, AT91C_BASE_CCFG + CCFG_EBI0CSA);

	/* Configure SDRAM Controller */
	sdram_init(	AT91C_SDRAMC_NC_9  |
				AT91C_SDRAMC_NR_13 |
				AT91C_SDRAMC_CAS_2 |
				AT91C_SDRAMC_NB_4_BANKS |
				AT91C_SDRAMC_DBW_32_BITS |
				AT91C_SDRAMC_TWR_2 |
				AT91C_SDRAMC_TRC_7 |
				AT91C_SDRAMC_TRP_2 |
				AT91C_SDRAMC_TRCD_2 |
				AT91C_SDRAMC_TRAS_5 |
				AT91C_SDRAMC_TXSR_8,		/* Control Register       */
				(MASTER_CLOCK * 7)/1000000,	/* Refresh Timer Register */
				AT91C_SDRAMC_MD_SDRAM);		/* SDRAM (no low power)   */ 
#endif	/* CFG_SDRAM */
}
#endif /* CFG_HW_INIT */


#ifdef CFG_SDRAM
//*----------------------------------------------------------------------------
//* \fn    sdramc_hw_init
//* \brief This function performs SDRAMC HW initialization
//*----------------------------------------------------------------------------*/
void sdramc_hw_init(void)
{
	/* Configure PIOs */
	const struct pio_desc sdramc_pio[] = {
		{"D16", AT91C_PIN_PD(16), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"D17", AT91C_PIN_PD(17), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"D18", AT91C_PIN_PD(18), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"D19", AT91C_PIN_PD(19), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"D20", AT91C_PIN_PD(20), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"D21", AT91C_PIN_PD(21), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"D22", AT91C_PIN_PD(22), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"D23", AT91C_PIN_PD(23), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"D24", AT91C_PIN_PD(24), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"D25", AT91C_PIN_PD(25), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"D26", AT91C_PIN_PD(26), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"D27", AT91C_PIN_PD(27), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"D28", AT91C_PIN_PD(28), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"D29", AT91C_PIN_PD(29), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"D30", AT91C_PIN_PD(30), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"D31", AT91C_PIN_PD(31), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{(char *) 0, 0, 0, PIO_DEFAULT, PIO_PERIPH_A},
	};

	/* Configure the SDRAMC PIO controller */
	pio_setup(sdramc_pio);
}
#endif

#ifdef CFG_DATAFLASH
/*------------------------------------------------------------------------------*/
/* \fn    df_hw_init								*/
/* \brief This function performs DataFlash HW initialization			*/
/*------------------------------------------------------------------------------*/
void df_hw_init(void)
{
	/* Configure PIOs */
	const struct pio_desc df_pio[] = {
		{"MISO",  AT91C_PIN_PA(0), 0, PIO_DEFAULT, PIO_PERIPH_B},
		{"MOSI",  AT91C_PIN_PA(1), 0, PIO_DEFAULT, PIO_PERIPH_B},
		{"SPCK",  AT91C_PIN_PA(2), 0, PIO_DEFAULT, PIO_PERIPH_B},
		{"NPCS0", AT91C_PIN_PA(5), 0, PIO_DEFAULT, PIO_PERIPH_B},
		{(char *) 0, 0, 0, PIO_DEFAULT, PIO_PERIPH_A},
	};

	/* Configure the PIO controller */
	pio_setup(df_pio);
}
#endif /* CFG_DATAFLASH */


#ifdef CFG_NANDFLASH
/*------------------------------------------------------------------------------*/
/* \fn    nandflash_hw_init							*/
/* \brief NandFlash HW init							*/
/*------------------------------------------------------------------------------*/
void nandflash_hw_init(void)
{
	/* Configure PIOs */
	const struct pio_desc nand_pio[] = {
		{"RDY_BSY", AT91C_PIN_PA(22), 0, PIO_PULLUP, PIO_INPUT},
		{"NANDCS",  AT91C_PIN_PD(15), 0, PIO_PULLUP, PIO_OUTPUT},
		{(char *) 0, 0, 0,  PIO_DEFAULT, PIO_PERIPH_A},
	};

	/* Setup Smart Media, first enable the address range of CS3 in HMATRIX user interface */
	writel(readl(AT91C_BASE_CCFG + CCFG_EBI0CSA) | AT91C_EBI_CS3A_SM, AT91C_BASE_CCFG + CCFG_EBI0CSA);
		    
	/* Configure SMC CS3 */
 	writel((AT91C_SM_NWE_SETUP | AT91C_SM_NCS_WR_SETUP | AT91C_SM_NRD_SETUP | AT91C_SM_NCS_RD_SETUP), AT91C_BASE_SMC0 + SMC_SETUP3);
  	writel((AT91C_SM_NWE_PULSE | AT91C_SM_NCS_WR_PULSE | AT91C_SM_NRD_PULSE | AT91C_SM_NCS_RD_PULSE), AT91C_BASE_SMC0 + SMC_PULSE3);
	writel((AT91C_SM_NWE_CYCLE | AT91C_SM_NRD_CYCLE)						, AT91C_BASE_SMC0 + SMC_CYCLE3);
	writel((AT91C_SMC_READMODE | AT91C_SMC_WRITEMODE | AT91C_SMC_NWAITM_NWAIT_DISABLE | 
  		AT91C_SMC_DBW_WIDTH_SIXTEEN_BITS | AT91C_SM_TDF)						, AT91C_BASE_SMC0 + SMC_CTRL3);

	/* Configure the PIO controller */
	writel((1 << AT91C_ID_PIOA), PMC_PCER + AT91C_BASE_PMC);
	writel((1 << AT91C_ID_PIOCDE), PMC_PCER + AT91C_BASE_PMC);

	pio_setup(nand_pio);
}

/*------------------------------------------------------------------------------*/
/* \fn    nandflash_cfg_16bits_dbw_init						*/
/* \brief Configure SMC in 16 bits mode						*/
/*------------------------------------------------------------------------------*/
void nandflash_cfg_16bits_dbw_init(void)
{
	writel(readl(AT91C_BASE_SMC0 + SMC_CTRL3) | AT91C_SMC_DBW_WIDTH_SIXTEEN_BITS, AT91C_BASE_SMC0 + SMC_CTRL3);
}

/*------------------------------------------------------------------------------*/
/* \fn    nandflash_cfg_8bits_dbw_init						*/
/* \brief Configure SMC in 8 bits mode						*/
/*------------------------------------------------------------------------------*/
void nandflash_cfg_8bits_dbw_init(void)
{
	writel((readl(AT91C_BASE_SMC0 + SMC_CTRL3) & ~(AT91C_SMC_DBW)) | AT91C_SMC_DBW_WIDTH_EIGTH_BITS, AT91C_BASE_SMC0 + SMC_CTRL3);
}

#endif /* #ifdef CFG_NANDFLASH */


