/***********************************************************
*  skeleton.c
*  Example for ping-pong processing
*  Caution: It is intended, that this file ist not runnable. 
*  The file contains mistakes and omissions, which shall be
*  corrected and completed by the students.
*
*   F. Quint, HsKA
************************************************************/


#include "SystemIdentificationcfg.h"
#include <csl.h>
#include <csl_mcbsp.h>
#include <csl_irq.h>
#include <csl_edma.h>
#include <dsk6713_led.h>
#include "config_AIC23.h"
#include "skeleton.h"
#include "tinymt32.h"
#include "math.h"



#define BUFFER_LEN 512
#define table_length 4
#define PI 3.14159265359
#define N_FIR_Filter 8
#define k 10000

float lp_coef[N_FIR_Filter+1] = {-0.001523,0.027550,-0.109033,0.222618,0.721350,0.222618,-0.109033,0.027550,-0.001523};
float baseband_signal_real[BUFFER_LEN]; //pase_buffer kann hier verwendet werden => 2 buffer sparen
float baseband_signal_imag[BUFFER_LEN];
float filtered_sig_real;
float filtered_sig_imag;
float phase[BUFFER_LEN];
float phase_diff;
float output;
/* Prototypenfunktionen anlegen */

void config_interrupts(void);





/* Ping-Pong buffers. Place them in the compiler section .datenpuffer */
/* How do you place the compiler section in the memory?     */
#pragma DATA_SECTION(Buffer_in_ping, ".datenpuffer");
short Buffer_in_ping[BUFFER_LEN];
#pragma DATA_SECTION(Buffer_in_pong, ".datenpuffer");
short Buffer_in_pong[BUFFER_LEN];
#pragma DATA_SECTION(Buffer_out_ping, ".datenpuffer");
short Buffer_out_ping[BUFFER_LEN];
#pragma DATA_SECTION(Buffer_out_pong, ".datenpuffer");
short Buffer_out_pong[BUFFER_LEN];

//Configuration for McBSP1 (data-interface)
MCBSP_Config datainterface_config = {

		/* McBSP Control Register */
        MCBSP_FMKS(SPCR, FREE, NO)              |	//  Freilauf
        MCBSP_FMKS(SPCR, SOFT, YES)          	|	// Clock stoppt beim debuggen (sofort)
        MCBSP_FMKS(SPCR, FRST, YES)             |	// Framesync ist ein
        MCBSP_FMKS(SPCR, GRST, YES)             |	// Reset aus, damit läuft der Samplerate- Generator
        MCBSP_FMKS(SPCR, XINTM, XRDY)          	|	// Sender Interrupt wird durch "XRDY-Bit" ausgelöst
        MCBSP_FMKS(SPCR, XSYNCERR, NO)          |	// empfängerseitig keine Überwachung der Synchronisation
        MCBSP_FMKS(SPCR, XRST, YES)             |	// Sender läuft (kein Reset- Status)
        MCBSP_FMKS(SPCR, DLB, OFF)              |	// Loopback (Kurschluss) nicht aktiv
        MCBSP_FMKS(SPCR, RJUST, RZF)            |	// rechtsbündige Ausrichtung der Daten im Puffer
        MCBSP_FMKS(SPCR, CLKSTP, DISABLE)       |	// Clock startet ohne Verzögerung auf fallenden Flanke (siehe auch PCR-Register)
        MCBSP_FMKS(SPCR, DXENA, OFF)            |	// DX- Enabler wird nicht verwendet
        MCBSP_FMKS(SPCR, RINTM, RRDY)           |	// Sender Interrupt wird durch "RRDY-Bit" ausgelöst
        MCBSP_FMKS(SPCR, RSYNCERR, NO)          |	// senderseitig keine Überwachung der Synchronisation
        MCBSP_FMKS(SPCR, RRST, YES),			// Empfänger läuft (kein Reset- Status)

		/* Empfangs-Control Register */
        MCBSP_FMKS(RCR, RPHASE, SINGLE)         |	// Nur eine Phase pro Frame
        MCBSP_FMKS(RCR, RFRLEN2, DEFAULT)       |	// Länge in Phase 2, unrelevant
        MCBSP_FMKS(RCR, RWDLEN2, DEFAULT)       |	// Wortlänge in Phase 2, unrelevant
        MCBSP_FMKS(RCR, RCOMPAND, MSB)          |	// kein Compandierung der Daten (MSB first)
        MCBSP_FMKS(RCR, RFIG, NO)               |	// Rahmensynchronisationspulse (nach dem ersten Puls)) startet die Übertragung neu
        MCBSP_FMKS(RCR, RDATDLY, 0BIT)          |	// keine Verzögerung (delay) der Daten
        MCBSP_FMKS(RCR, RFRLEN1, OF(1))         |	// Länge der Phase 1 --> 1 Wort
        MCBSP_FMKS(RCR, RWDLEN1, 16BIT)         |	// Wortlänge in Phase 1, 16Bit = 1 Wort
        MCBSP_FMKS(RCR, RWDREVRS, DISABLE),		// 32-bit Reversal nicht genutzt

		/* Sende-Control Register */
        MCBSP_FMKS(XCR, XPHASE, SINGLE)        	|	// ? Phase 2 unrelevant ?
        MCBSP_FMKS(XCR, XFRLEN2, DEFAULT)       |	// Länge in Phase 2, unrelevant
        MCBSP_FMKS(XCR, XWDLEN2, DEFAULT)       |	// Wortlänge in Phase 2, unrelevant
        MCBSP_FMKS(XCR, XCOMPAND, MSB)          |	// kein Compandierung der Daten (MSB first)
        MCBSP_FMKS(XCR, XFIG, NO)               |	// Rahmensynchronisationspulse (nach dem ersten Puls)) startet die Übertragung neu
        MCBSP_FMKS(XCR, XDATDLY, 0BIT)          |	// keine Verzögerung (delay) der Daten
        MCBSP_FMKS(XCR, XFRLEN1, OF(1))         |	// Länge der Phase 1 --> 1 Wort
        MCBSP_FMKS(XCR, XWDLEN1, 16BIT)         |	// Wortlänge in Phase 1 --> 16 bit
        MCBSP_FMKS(XCR, XWDREVRS, DISABLE),		// 32-bit Reversal nicht genutzt

		/* Sample Rate Generator Register */
        MCBSP_FMKS(SRGR, GSYNC, DEFAULT)        |	// Einstellungen nicht relevant da
        MCBSP_FMKS(SRGR, CLKSP, DEFAULT)        |	// der McBSP1 als Slave läuft
        MCBSP_FMKS(SRGR, CLKSM, DEFAULT)        |	// und den Takt von aussen
        MCBSP_FMKS(SRGR, FSGM, DXR2XSR)        	|	// vorgegeben bekommt.
        MCBSP_FMKS(SRGR, FPER, DEFAULT)         |	// --
        MCBSP_FMKS(SRGR, FWID, DEFAULT)         |	// --
        MCBSP_FMKS(SRGR, CLKGDV, DEFAULT),		// --
		/* Mehrkanal */
        MCBSP_MCR_DEFAULT,				// Mehrkanal wird nicht verwendet
        MCBSP_RCER_DEFAULT,				// dito
        MCBSP_XCER_DEFAULT,				// dito
		/* Pinout Control Register */
        MCBSP_FMKS(PCR, XIOEN, SP)              |	// Pin wird für serielle Schnittstelle verwendet (alternativ GPIO)
        MCBSP_FMKS(PCR, RIOEN, SP)              |	// Pin wird für serielle Schnittstelle verwendet (alternativ GPIO)
        MCBSP_FMKS(PCR, FSXM, EXTERNAL)         |	// Framesync- Signal für Sender kommt von extern (Slave)
        MCBSP_FMKS(PCR, FSRM, EXTERNAL)         |	// Framesync- Signal für Empfänger kommt von extern (Slave)
        MCBSP_FMKS(PCR, CLKXM, INPUT)           |	// Takt für Sender kommt von extern (Slave)
        MCBSP_FMKS(PCR, CLKRM, INPUT)           |	// Takt für Empfänger kommt von extern (Slave)
        MCBSP_FMKS(PCR, CLKSSTAT, DEFAULT)      |	// unrelevant da PINS keine GPIOs
        MCBSP_FMKS(PCR, DXSTAT, DEFAULT)        |	// unrelevant da PINS keine GPIOs
        MCBSP_FMKS(PCR, FSXP, ACTIVEHIGH)       |	// Framesync senderseitig ist "activehigh"
        MCBSP_FMKS(PCR, FSRP, ACTIVEHIGH)       |	// Framesync empfängerseitig ist "activehigh"
        MCBSP_FMKS(PCR, CLKXP, FALLING)         |	// Datum wird bei fallender Flanke gesendet
        MCBSP_FMKS(PCR, CLKRP, RISING)			// Datum wird bei steigender Flanke übernommen
};


/* template for a EDMA configuration */
EDMA_Config configEDMARcv = {
    EDMA_FMKS(OPT, PRI, LOW)          |  // auf beide Queues verteilen
    EDMA_FMKS(OPT, ESIZE, 16BIT)       |  // Element size, von RWDLEN1 übernommen
    EDMA_FMKS(OPT, 2DS, NO)            |  // kein 2D-Transfer
    EDMA_FMKS(OPT, SUM, NONE)          |  // Quell-update mode -> FEST (McBSP)!!!
    EDMA_FMKS(OPT, 2DD, NO)            |  // 2kein 2D-Transfer
    EDMA_FMKS(OPT, DUM, INC)           |  // Ziel-update mode, hochzählen
    EDMA_FMKS(OPT, TCINT, YES)         |  // EDMA interrupt erzeugen?, ja!
    EDMA_FMKS(OPT, TCC, OF(0))         |  // Transfer complete code (TCC)
    EDMA_FMKS(OPT, LINK, YES)          |  // Link Parameter nutzen?
    EDMA_FMKS(OPT, FS, NO),               // Frame Sync nutzen?

    EDMA_FMKS(SRC, SRC, OF(0)),           // Quell-Adresse

    EDMA_FMK (CNT, FRMCNT, 0)	       |  // Anzahl Frames
    EDMA_FMK (CNT, ELECNT, BUFFER_LEN),   // Anzahl Elemente

    (Uint32)Buffer_in_ping,       		  // Ziel-Adresse

    EDMA_FMKS(IDX, FRMIDX, DEFAULT)    |  // Frame index Wert
    EDMA_FMKS(IDX, ELEIDX, DEFAULT),      // Element index Wert

    EDMA_FMK (RLD, ELERLD, 0)       	|  // Reload Element
    EDMA_FMK (RLD, LINK, 0)            // Reload Link
};

EDMA_Config configEDMAXmt = {
    EDMA_FMKS(OPT, PRI, LOW)          |  // auf beide Queues verteilen
    EDMA_FMKS(OPT, ESIZE, 16BIT)       |  // Element size, von RWDLEN1 übernommen
    EDMA_FMKS(OPT, 2DS, NO)            |  // kein 2D-Transfer
    EDMA_FMKS(OPT, SUM, INC)           |  // Quell-update mode, hochzählen
    EDMA_FMKS(OPT, 2DD, NO)            |  // 2kein 2D-Transfer
    EDMA_FMKS(OPT, DUM, NONE)          |  // Ziel-update mode -> FEST (McBSP)!!!
    EDMA_FMKS(OPT, TCINT, YES)         |  // EDMA interrupt erzeugen?, ja!
    EDMA_FMKS(OPT, TCC, OF(0))         |  // Transfer complete code (TCC)
    EDMA_FMKS(OPT, LINK, YES)          |  // Link Parameter nutzen?
    EDMA_FMKS(OPT, FS, NO),               // Frame Sync nutzen?

    (Uint32)Buffer_out_ping,           // Quell-Adresse

    EDMA_FMK (CNT, FRMCNT, 0)	       |  // Anzahl Frames
    EDMA_FMK (CNT, ELECNT, BUFFER_LEN),   // Anzahl Elemente

	EDMA_FMKS(DST, DST, OF(0)),    		  // Ziel-Adresse

    EDMA_FMKS(IDX, FRMIDX, DEFAULT)    |  // Frame index Wert
    EDMA_FMKS(IDX, ELEIDX, DEFAULT),      // Element index Wert

    EDMA_FMK (RLD, ELERLD, 0)       	|  // Reload Element
    EDMA_FMK (RLD, LINK, 0)            // Reload Link
};




/* Transfer-Complete-Codes for EDMA-Jobs */
int tccRcvPing = 0;
int tccRcvPong = 0;
int tccXmtPing = 0;
int tccXmtPong = 0;

/* EDMA-Handles; are these really all? */
EDMA_Handle hEdmaRcv;  
EDMA_Handle hEdmaReload; 
EDMA_Handle hEdmaXmt;
EDMA_Handle hEdmaReloadRcvPing;
EDMA_Handle hEdmaReloadXmtPing;
EDMA_Handle hEdmaReloadRcvPong;
EDMA_Handle hEdmaReloadXmtPong;

MCBSP_Handle hMcbsp;



								
main()
{
	//MCBSP_Handle hMcbsp=0;


	CSL_init();  
	
	/* Configure McBSP0 and AIC23 */
	Config_DSK6713_AIC23();
	
	/* Configure McBSP1*/
	hMcbsp = MCBSP_open(MCBSP_DEV1, MCBSP_OPEN_RESET);
    MCBSP_config(hMcbsp, &datainterface_config);
    IRQ_globalDisable();
    
	/* configure EDMA */
    config_EDMA();

    /* finally the interrupts */
    config_interrupts();


    MCBSP_start(hMcbsp, MCBSP_RCV_START | MCBSP_XMIT_START, 0xffffffff);		// Starten beim Lesen oder Schreiben
    MCBSP_write(hMcbsp, 0x0); 	/* one shot */


} /* finished*/



void config_EDMA(void)
{
	/* Konfiguration der EDMA zum Lesen*/
		/* Für Ping */
		hEdmaRcv = EDMA_open(EDMA_CHA_REVT1, EDMA_OPEN_RESET);  // EDMA Channel for REVT1
		hEdmaReloadRcvPing = EDMA_allocTable(-1);               // Reload-Parameters
		hEdmaReloadRcvPong = EDMA_allocTable(-1);               // Reload-Parameters

		tccRcvPing = EDMA_intAlloc(-1);                        // next available TCC
		configEDMARcv.src = MCBSP_getRcvAddr(hMcbsp);          // source addr
		configEDMARcv.opt |= EDMA_FMK(OPT,TCC,tccRcvPing);     // set it
		configEDMARcv.dst = EDMA_DST_OF(Buffer_in_ping);	   // Set destination

		/* configure */
		EDMA_config(hEdmaRcv, &configEDMARcv);
		EDMA_config(hEdmaReloadRcvPing, &configEDMARcv);
		/* could we need also some other EDMA read job?*/

		/* Für Pong */
		tccRcvPong = EDMA_intAlloc(-1);                        // next available TCC
		configEDMARcv.src = MCBSP_getRcvAddr(hMcbsp);          // source addr
		configEDMARcv.opt |= EDMA_FMK(OPT,TCC,tccRcvPong);     // set it
		configEDMARcv.dst = EDMA_DST_OF(Buffer_in_pong);	   // Set destination

		/* configure */
		EDMA_config(hEdmaReloadRcvPong, &configEDMARcv);

		/* link transfers ping -> pong -> ping */
		EDMA_link(hEdmaRcv,hEdmaReloadRcvPong);  /* is that all? */
		EDMA_link(hEdmaReloadRcvPong, hEdmaReloadRcvPing);
		EDMA_link(hEdmaReloadRcvPing, hEdmaReloadRcvPong);


	/* Konfiguration der EDMA zum Schreiben*/
		/* Für Ping */
		hEdmaXmt = EDMA_open(EDMA_CHA_XEVT1, EDMA_OPEN_RESET);  // EDMA Channel for XEVT1
		hEdmaReloadXmtPing = EDMA_allocTable(-1);               // Reload-Parameters
		hEdmaReloadXmtPong = EDMA_allocTable(-1);				// Reload-Parameters

		tccXmtPing = EDMA_intAlloc(-1);                        // next available TCC
		configEDMAXmt.dst = MCBSP_getXmtAddr(hMcbsp);          // destination addr
		configEDMAXmt.opt |= EDMA_FMK(OPT,TCC,tccXmtPing);     // set it
		configEDMAXmt.src = EDMA_DST_OF(Buffer_out_ping);		   // Set source

		/* configure */
		EDMA_config(hEdmaXmt, &configEDMAXmt);
		EDMA_config(hEdmaReloadXmtPing, &configEDMAXmt);

		/* Für Pong */
		tccXmtPong = EDMA_intAlloc(-1);                        // next available TCC
		configEDMAXmt.dst = MCBSP_getXmtAddr(hMcbsp);          // destination addr
		configEDMAXmt.opt |= EDMA_FMK(OPT,TCC,tccXmtPong);     // set it
		configEDMAXmt.src = EDMA_DST_OF(Buffer_out_pong);	   // Set source

		/* configure */
		EDMA_config(hEdmaReloadXmtPong, &configEDMAXmt);

	/* link transfers ping -> pong -> ping */
	EDMA_link(hEdmaXmt, hEdmaReloadXmtPong);
	EDMA_link(hEdmaReloadXmtPong, hEdmaReloadXmtPing);
	EDMA_link(hEdmaReloadXmtPing, hEdmaReloadXmtPong);


	/* do you want to hear music? */


	/* enable EDMA TCC */
	EDMA_intClear(tccRcvPing);
	EDMA_intEnable(tccRcvPing);

	EDMA_intClear(tccXmtPing);
	EDMA_intEnable(tccXmtPing);

	EDMA_intClear(tccRcvPong);
	EDMA_intEnable(tccRcvPong);

	EDMA_intClear(tccXmtPong);
	EDMA_intEnable(tccXmtPong);
	/* some more? */

	/* which EDMAs do we have to enable? */
	EDMA_enableChannel(hEdmaRcv);
	EDMA_enableChannel(hEdmaXmt);
}



void config_interrupts(void)
{
	IRQ_map(IRQ_EVT_EDMAINT, 8);		// EDMAInterupt mit CPU Interupt 8 verbinden
	IRQ_clear(IRQ_EVT_EDMAINT);
	IRQ_enable(IRQ_EVT_EDMAINT);
	SWI_enable();
	IRQ_globalEnable();
}


void EDMA_interrupt_service(void)
{
	static int RcvPingDone=0;
	static int RcvPongDone=0;
	static int XmtPingDone=0;
	static int XmtPongDone=0;

	if(EDMA_intTest(tccRcvPing)) {
		EDMA_intClear(tccRcvPing); /* clear is mandatory */
		RcvPingDone=1;
	}
	else if(EDMA_intTest(tccRcvPong)) {
		EDMA_intClear(tccRcvPong);
		RcvPongDone=1;
	}
	
	// Für Xmt auch
	if(EDMA_intTest(tccXmtPing)) {
			EDMA_intClear(tccXmtPing); /* clear is mandatory */
			XmtPingDone=1;
		}
		else if(EDMA_intTest(tccXmtPong)) {
			EDMA_intClear(tccXmtPong);
			XmtPongDone=1;
		}
	
	if(RcvPingDone && XmtPingDone) {
		RcvPingDone=0;
		XmtPingDone=0;
		// processing in SWI
		SWI_post(&SWI_process_ping);
	}
	else if(RcvPongDone && XmtPongDone) {
		RcvPongDone=0;
		XmtPongDone=0;
		// processing in SWI
		SWI_post(&SWI_process_pong);
	}
}

void process_ping_SWI(void)
{
	int i,j,index;
	for(i=0; i<BUFFER_LEN; i++)
	{
		// cos_lut[table_length]    = { 1,0,-1,0 };
		// -i*sin_lut[table_length] = { 0,-1,0,1 };
		// baseband = sig*(cos-i*sin)
		if (i%table_length == 0)
		{
			*(baseband_signal_real + i) = *(Buffer_in_ping + i) ;
			*(baseband_signal_imag + i) = 0;
		}else if (i%table_length == 1)
		{
			*(baseband_signal_real + i) = 0;
			*(baseband_signal_imag + i) = -*(Buffer_in_ping + i);
		}else if (i%table_length == 2)
		{
			*(baseband_signal_real + i) = -*(Buffer_in_ping + i);
			*(baseband_signal_imag + i) = 0;
		}else if (i%table_length == 3)
		{
			*(baseband_signal_real + i) = 0;
			*(baseband_signal_imag + i) = *(Buffer_in_ping + i);
		}

		filtered_sig_real = 0;
		filtered_sig_imag = 0;
		for (j = 0; j < N_FIR_Filter; j++)
		{
			if (i < N_FIR_Filter)
			{
				index = BUFFER_LEN +(i-j);
			}
			else
			{
				index = (i-j);
			}
			filtered_sig_real += (lp_coef[j] * (*(baseband_signal_real + index)));
			filtered_sig_imag += (lp_coef[j] * (*(baseband_signal_imag + index)));
		}
		//phase calculation phase = arctan(Im / Re)
		*(phase + i) = 0;
		//im_durch_re = *(filtered_sig_imag + ii) / *(filtered_sig_real + ii);
		*(phase + i) = atan2(filtered_sig_imag,filtered_sig_real);

		if (i >= N_FIR_Filter) phase_diff = (*(phase + i) - *(phase + i - 1));
		else phase_diff = 0;
		// unwrap
		while (abs(phase_diff)  > PI)
		{
			if (phase_diff> 0) phase_diff -= 2*PI;
			else phase_diff += 2*PI;
		}
		output = k*phase_diff;
		*(Buffer_out_ping + i) = output*0.05;
		//*(Buffer_out_ping+i) = *(Buffer_in_ping+i);
	}
}

void process_pong_SWI(void)
{
	int i,j,index;
	for(i=0; i<BUFFER_LEN; i++)
	{
		// cos_lut[table_length]    = { 1,0,-1,0 };
		// -i*sin_lut[table_length] = { 0,-1,0,1 };
		// baseband = sig*(cos-i*sin)
		if (i%table_length == 0)
		{
			*(baseband_signal_real + i) = *(Buffer_in_pong + i) ;
			*(baseband_signal_imag + i) = 0;
		}else if (i%table_length == 1)
		{
			*(baseband_signal_real + i) = 0;
			*(baseband_signal_imag + i) = -*(Buffer_in_pong + i);
		}else if (i%table_length == 2)
		{
			*(baseband_signal_real + i) = -*(Buffer_in_pong + i);
			*(baseband_signal_imag + i) = 0;
		}else if (i%table_length == 3)
		{
			*(baseband_signal_real + i) = 0;
			*(baseband_signal_imag + i) = *(Buffer_in_pong + i);
		}

		filtered_sig_real = 0;
		filtered_sig_imag = 0;
		for (j = 0; j < N_FIR_Filter; j++)
		{
			if (i < N_FIR_Filter)
			{
				index = BUFFER_LEN +(i-j);
			}
			else
			{
				index = (i-j);
			}
			filtered_sig_real += (lp_coef[j] * (*(baseband_signal_real + index)));
			filtered_sig_imag += (lp_coef[j] * (*(baseband_signal_imag + index)));
		}
		//phase calculation phase = arctan(Im / Re)
		*(phase + i) = 0;
		//im_durch_re = *(filtered_sig_imag + ii) / *(filtered_sig_real + ii);
		*(phase + i) = atan2(filtered_sig_imag,filtered_sig_real);
		if (i >= N_FIR_Filter) phase_diff = (*(phase + i) - *(phase + i - 1));
		else phase_diff = 0;
		// unwrap
		while (abs(phase_diff)  > PI)
		{
			if (phase_diff> 0) phase_diff -= 2*PI;
			else phase_diff += 2*PI;
		}
		output = k*phase_diff;
		*(Buffer_out_pong + i) = output*0.05;
		//*(Buffer_out_pong+i) = *(Buffer_in_pong+i);
	}
}
void SWI_LEDToggle(void)
{
	SEM_postBinary(&SEM_LEDToggle);	
}

void tsk_led_toggle(void)
{
	/* initializatoin of the task */
	/* nothing to do */

	/* process */
	while(1) {
		SEM_pendBinary(&SEM_LEDToggle, SYS_FOREVER);

		DSK6713_LED_toggle(1);
	}
}
