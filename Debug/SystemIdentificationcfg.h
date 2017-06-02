/*   Do *not* directly modify this file.  It was    */
/*   generated by the Configuration Tool; any  */
/*   changes risk being overwritten.                */

/* INPUT SystemIdentification.cdb */

/*  Include Header Files  */
#include <std.h>
#include <prd.h>
#include <hst.h>
#include <swi.h>
#include <tsk.h>
#include <log.h>
#include <sem.h>
#include <sts.h>

#ifdef __cplusplus
extern "C" {
#endif

extern far PRD_Obj PRD_LED_Toggel;
extern far HST_Obj RTA_fromHost;
extern far HST_Obj RTA_toHost;
extern far SWI_Obj PRD_swi;
extern far SWI_Obj KNL_swi;
extern far SWI_Obj SWI_process_ping;
extern far SWI_Obj SWI_process_pong;
extern far TSK_Obj TSK_idle;
extern far TSK_Obj led_toggle_tsk;
extern far LOG_Obj LOG_system;
extern far SEM_Obj SEM_LEDToggle;
extern far STS_Obj IDL_busyObj;


#ifdef __cplusplus
}
#endif /* extern "C" */
