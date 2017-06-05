
#include <std.h>
#include <stdlib.h>
#include <csl.h>
#include <csl_mcbsp.h>
#include <csl_irq.h>
#include <csl_edma.h>
#include <dsk6713_led.h>
#include "config_AIC23.h"
#include "skeleton.h"
#include <log.h>
#include "DspBiosConfigurationcfg.h"
extern far LOG_Obj myLog;

#define BUFFER_LEN 500
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
        MCBSP_FMKS(SPCR, SOFT, NO)          	|   //	##########	beim Debuggen stoppt auch die Übertragung bei einem Stop
													//  // ###### Soft mode is disabled. Serial port clock stops immediately
        MCBSP_FMKS(SPCR, FRST, YES)             |	// Framesync ist ein
        MCBSP_FMKS(SPCR, GRST, YES)             |	// Reset aus, damit läuft der Samplerate- Generator
        MCBSP_FMKS(SPCR, XINTM, XRDY)          	|   //	########## XINT is generated on every serial element by tracking the XRDY bits in SPCR (Interrupt in EDMA)
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
        MCBSP_FMKS(RCR, RWDLEN1, 16BIT)         |	//##########	Recieve Word Length on phase 1: 1 Word
        MCBSP_FMKS(RCR, RWDREVRS, DISABLE),		// 32-bit Reversal nicht genutzt

        /* Sende-Control Register */
        MCBSP_FMKS(XCR, XPHASE, SINGLE)         |	// ##############		Nur eine Phase pro Frame
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
        MCBSP_FMKS(SRGR, FSGM, DEFAULT)         |	// vorgegeben bekommt.
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
    EDMA_FMKS(OPT, ESIZE, 16BIT)       |  	// Element size					:		// Auflösung des ADC 16 bit Im Foliensatz so gefunden
    EDMA_FMKS(OPT, 2DS, NO)            |  // kein 2D-Transfer
    EDMA_FMKS(OPT, SUM, NONE)          |  // Quell-update mode -> FEST (McBSP)!!!
    EDMA_FMKS(OPT, 2DD, NO)            |  // 2kein 2D-Transfer
    EDMA_FMKS(OPT, DUM, INC)           |  // Ziel-update mode				// ######## Source.update mde:		Quell-Adresse wird nicht inkrementiert (Buffer)
    EDMA_FMKS(OPT, TCINT,YES)         |  // EDMA interrupt erzeugen?		//	########	EDMA muss Interrupts erzeugen!
    EDMA_FMKS(OPT, TCC, OF(0))         |  // Transfer complete code (TCC)
    EDMA_FMKS(OPT, LINK, YES)          |  // Link Parameter nutzen?
    EDMA_FMKS(OPT, FS, NO),               // Frame Sync nutzen?

    EDMA_FMKS(SRC, SRC, OF(0)),           // Quell-Adresse

    EDMA_FMK (CNT, FRMCNT, NULL)       |  // Anzahl Frames
    EDMA_FMK (CNT, ELECNT, BUFFER_LEN),   // Anzahl Elemente

    (Uint32)Buffer_in_ping,       		  // Ziel-Adresse

    EDMA_FMKS(IDX, FRMIDX, DEFAULT)    |  // Frame index Wert
    EDMA_FMKS(IDX, ELEIDX, DEFAULT),      // Element index Wert

    EDMA_FMK (RLD, ELERLD, NULL)       |  // Reload Element
    EDMA_FMK (RLD, LINK, NULL)            // Reload Link
};

/* ########################################################################################################### */

EDMA_Config configEDMAXmt =						/* Parameter nochmal überprüfen!!!!!!! */
{
    EDMA_FMKS(OPT, PRI, LOW)        	|	// auf beide Queues verteilen
    EDMA_FMKS(OPT, ESIZE, 16BIT)		|		// Element size					ADC 16bit		wegen buffer-in
    EDMA_FMKS(OPT, 2DS, NO)				|	// kein 2D-Transfer
    EDMA_FMKS(OPT, SUM, INC)			|	// Quell-update mode -> FEST (McBSP)!!!
    EDMA_FMKS(OPT, 2DD, NO)				|	// 2kein 2D-Transfer
    EDMA_FMKS(OPT, DUM, NONE)			|		// Ziel-update mode	############		Ziel-Adresse wird aktualisiert (inkrementiert)
    EDMA_FMKS(OPT, TCINT, YES)			|		// EDMA interrupt erzeugen?	############	EDMA muss Interrupts erzeugen!
    EDMA_FMKS(OPT, TCC, OF(0))			|	// Transfer complete code (TCC)
    EDMA_FMKS(OPT, LINK, YES)			|	// Link Parameter nutzen?
    EDMA_FMKS(OPT, FS, NO),             	// Frame Sync nutzen?

    EDMA_FMKS(SRC, SRC, OF(0)),				// Quell-Adresse

    EDMA_FMK (CNT, FRMCNT, NULL)		|	// Anzahl Frames
    EDMA_FMK (CNT, ELECNT, BUFFER_LEN),		// Anzahl Elemente

    (Uint32)Buffer_in_ping,					// Ziel-Adresse							Zur Laufzeit zur Pong-Adresse ändern

    EDMA_FMKS(IDX, FRMIDX, DEFAULT)		|	// Frame index Wert
    EDMA_FMKS(IDX, ELEIDX, DEFAULT),		// Element index Wert

    EDMA_FMK (RLD, ELERLD, NULL)		|	// Reload Element
    EDMA_FMK (RLD, LINK, NULL)				// Reload Link
};

TIMER_Config configTIMER1 = {
		 0x00000201, //CTL Timer Control Register
		 /*
		  *
		  *  XXXXXXXXXXXXXXXXXXXX
		  *  0	TSTAT
		  *  0	INVINP   Noninverted TINP drives timer.
		  *  1	CLKSRC  Internal clock source CPU clock/4
		  *  0	CP	Pulse mode. TSTAT is active one CPU clock after the timer reaches the timer period
		  *  0	HLD Counter is disabled and held in the current state !!PLAY/PAUSE!!
		  *  0	GO  Resets Timer if 1 and starts to count when HLD 1
		  *  X 	Reserved
		  *  0	PWID  Pulse width bit
		  *  0	DATIN
		  *  0	DATOUT
		  *  0	INVOUT Noninverted TSTAT drives TOUT
		  *  1	FUNC TOUT=Timer Output Pin
		  */

		 0xFFFFFFFF, //PRD Timer Period Register
		 0x00000000  //CNT Timer Count Register
};



/* ########################################################################################################### */


/* Transfer-Complete-Codes for EDMA-Jobs */
int tccRcvPing;
int tccRcvPong;
int tccXmtPing;
int tccXmtPong;

/* EDMA-Handles */
EDMA_Handle hEdmaRcv;  
EDMA_Handle hEdmaReloadRcvPing;
EDMA_Handle hEdmaReloadRcvPong;

EDMA_Handle hEdmaXmt;
EDMA_Handle hEdmaReloadXmtPing;
EDMA_Handle hEdmaReloadXmtPong;

MCBSP_Handle hMcbsp;
TIMER_Handle timer1; //Because timer0 is already in use for the led
						
void config_interrupts(void);
main()
{
	//LOG_printf(&myLog, "main begin");
	MCBSP_Handle hMcbsp=0;
	
	CSL_init();  
	
	/* Configure McBSP0 and AIC23 */
	Config_DSK6713_AIC23();
	
	/* Configure McBSP1*/
	hMcbsp = MCBSP_open(MCBSP_DEV1, MCBSP_OPEN_RESET);
    MCBSP_config(hMcbsp, &datainterface_config);
    
	/* configure EDMA */
    config_EDMA();

    /* finally the interrupts */
    config_interrupts();

    MCBSP_start(hMcbsp, MCBSP_RCV_START | MCBSP_XMIT_START | MCBSP_SRGR_START | MCBSP_SRGR_FRAMESYNC, 220);		//	#####	Start Field: Recieve
    																			//				Es gibt auch noch MCBSP_SRGR_START (start sample Rate Generator) und MCBSP_SRGR_FRAMESYNC (start frame sync. generation)
    MCBSP_write(hMcbsp, 0x0); 	/* one shot */

    timer1 = TIMER_open(TIMER_DEV1,TIMER_OPEN_RESET);
    TIMER_config(timer1, &configTIMER1);

} /* finished*/



void config_EDMA(void)
{
		LOG_printf(&myLog, "config EDMA begin");

		/* Konfiguration der EDMA zum Lesen*/
		hEdmaRcv = EDMA_open(EDMA_CHA_REVT1, EDMA_OPEN_RESET);  	// EDMA Channel for REVT1
		hEdmaReloadRcvPing = EDMA_allocTable(-1);               	// Reload-Parameters Ping
		hEdmaReloadRcvPong = EDMA_allocTable(-1);               	// Reload-Parameters Pong				2 Param Sets needed (Ping & Pong) -> Pong write on my own

		configEDMARcv.src = MCBSP_getRcvAddr(hMcbsp);				//  source addr of mcbsp 1 to get data

		//Receive Ping
		tccRcvPing = EDMA_intAlloc(-1);                        		// next available TCC
		configEDMARcv.opt |= EDMA_FMK(OPT,TCC,tccRcvPing);			// Grundkonfiguration EDMA-channel
		configEDMARcv.dst = ((int)Buffer_in_ping)/* & 0xFFFF*/;			// Zieladresse

		/* Erster Transfer und Reload-Ping */
		EDMA_config(hEdmaRcv, &configEDMARcv);
		EDMA_config(hEdmaReloadRcvPing, &configEDMARcv);

		//Receive Pong
		tccRcvPong = EDMA_intAlloc(-1);                        		// next available TCC
		configEDMARcv.opt |= EDMA_FMK(OPT,TCC,tccRcvPong);			// Grundkonfiguration EDMA-channel
		configEDMARcv.dst = ((int)Buffer_in_pong)/* & 0xFFFF*/;			// Zieladresse

		/* Erster Transfer und Reload-Ping */
		EDMA_config(hEdmaReloadRcvPong, &configEDMARcv);

		/* link transfers ping -> pong -> ping */
		EDMA_link(hEdmaRcv,hEdmaReloadRcvPong);
		EDMA_link(hEdmaReloadRcvPong,hEdmaReloadRcvPing);
		EDMA_link(hEdmaReloadRcvPing,hEdmaReloadRcvPong);

		// Gleiche Konfiguration wie oben für Lesen muss nun für Schreiben angewendet werden

		hEdmaXmt = EDMA_open(EDMA_CHA_XEVT1, EDMA_OPEN_RESET);  	// EDMA Channel for REVT1
		hEdmaReloadXmtPing = EDMA_allocTable(-1);               	// Reload-Parameters Ping
		hEdmaReloadXmtPong = EDMA_allocTable(-1);               	// Reload-Parameters Pong					2 Param Sets needed (Ping & Pong) -> Pong write on my own

		configEDMAXmt.dst = MCBSP_getXmtAddr(hMcbsp);				//  destination addr

		//Transmit Ping
		tccXmtPing = EDMA_intAlloc(-1);                        		// next available TCC
		configEDMAXmt.opt |= EDMA_FMK(OPT,TCC,tccXmtPing);			// Grundkonfiguration EDMA-channel
		configEDMAXmt.src = ((int)Buffer_out_ping)/* & 0xFFFF*/;		// Quelladresse

		/* Erster Transfer und Reload-Ping */
		EDMA_config(hEdmaXmt, &configEDMAXmt);
		EDMA_config(hEdmaReloadXmtPing, &configEDMAXmt);

		//Transmit Pong
		tccXmtPong = EDMA_intAlloc(-1);                        		// next available TCC
		configEDMAXmt.opt |= EDMA_FMK(OPT,TCC,tccXmtPong);			// Grundkonfiguration EDMA-channel
		configEDMAXmt.src = ((int)Buffer_out_pong)/* & 0xFFFF*/;		// Quelladresse

		/* Erster Transfer und Reload-Ping */
		EDMA_config(hEdmaReloadXmtPong, &configEDMAXmt);

		/* link transfers ping -> pong -> ping */
		EDMA_link(hEdmaXmt,hEdmaReloadXmtPong);
		EDMA_link(hEdmaReloadXmtPong,hEdmaReloadXmtPing);
		EDMA_link(hEdmaReloadXmtPing,hEdmaReloadXmtPong);

		/* enable EDMA TCC */
		//receive ping
		EDMA_intClear(tccRcvPing);
		EDMA_intEnable(tccRcvPing);
		//receive pong
		EDMA_intClear(tccRcvPong);
		EDMA_intEnable(tccRcvPong);
		//transmit ping
		EDMA_intClear(tccXmtPing);
		EDMA_intEnable(tccXmtPing);
		//transmit pong
		EDMA_intClear(tccXmtPong);
		EDMA_intEnable(tccXmtPong);

		/* which EDMAs */
		EDMA_enableChannel(hEdmaRcv);
		EDMA_enableChannel(hEdmaXmt);

		//LOG_printf(&myLog, "config EDMA end");
}


void config_interrupts(void)
{
	//LOG_printf(&myLog, "config interrupts begin");

	IRQ_map(IRQ_EVT_EDMAINT, 8);
	IRQ_clear(IRQ_EVT_EDMAINT);
	IRQ_enable(IRQ_EVT_EDMAINT);
	IRQ_globalEnable();

	//LOG_printf(&myLog, "config interrupts end");
}


void EDMA_interrupt_service(void)
{
	//LOG_printf(&myLog, "EDMA interrupt");

	static volatile int rcvPingDone=0;
	static volatile int rcvPongDone=0;
	static volatile int xmtPingDone=0;
	static volatile int xmtPongDone=0;
	
	if(EDMA_intTest(tccRcvPing))
	{
		EDMA_intClear(tccRcvPing);		/* clear */
		rcvPingDone=1;
	}
	else if(EDMA_intTest(tccRcvPong))				//	############
	{
		EDMA_intClear(tccRcvPong);
		rcvPongDone=1;
	}
	if(EDMA_intTest(tccXmtPing))				//	##############
	{
		EDMA_intClear(tccXmtPing);
		xmtPingDone=1;
	}
	else if(EDMA_intTest(tccXmtPong))				//	################
	{
		EDMA_intClear(tccXmtPong);
		xmtPongDone=1;
	}
	
	if(rcvPingDone && xmtPingDone) {
		rcvPingDone=0;
		xmtPingDone=0;
		// processing in SWI
		SWI_post(&Ping_SWI);
	}
	else if(rcvPongDone && xmtPongDone) {
		rcvPongDone=0;
		xmtPongDone=0;
		// processing in SWI
		SWI_post(&Pong_SWI);
	}
}

/* ##########################################################################*/
void process_ping_SWI(void)
{
/*	int i;
	for(i=0; i<BUFFER_LEN; i++)
		*(Buffer_out_ping+i) = *(Buffer_in_ping+i); */
}

void process_pong_SWI(void)
{
/*	int i;
	for(i=0; i<BUFFER_LEN; i++)
		*(Buffer_out_pong+i) = *(Buffer_in_pong+i); */
}

/* ##########################################################################*/

void SWI_LEDToggle(void)
{
	//SEM_postBinary(&SEM_LEDToggle);
}

void tsk_led_toggle(void)
{
	/* initializatoin of the task */
	/* nothing to do */
	/* process */
	while(1) {
		SEM_pendBinary(&SEM_LEDToggle, SYS_FOREVER);
		
		//DSK6713_LED_toggle(1);
		//DSK6713_LED_off(3);
		//DSK6713_LED_toggle(1);
		//DSK6713_LED_toggle(0);
		//DSK6713_LED_off(2);
	}
}
