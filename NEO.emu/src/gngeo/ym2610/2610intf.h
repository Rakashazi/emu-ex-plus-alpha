#ifndef __2610INTF_H__
#define __2610INTF_H__

#include <gngeoTypes.h>

#ifdef HAVE_CONFIG_H
#include <gngeo-config.h>
#endif

#include "ym2610.h"

#define YM2610UpdateRequest()

/************************************************/
/* Sound Hardware Start				*/
/************************************************/
int YM2610_sh_start(void);

/************************************************/
/* Sound Hardware Stop				*/
/************************************************/
void YM2610_sh_stop(void);

void YM2610_sh_reset(void);
void timer_callback_2610(int param);

/************************************************/
/* Chip 0 functions								*/
/************************************************/
Uint32 YM2610_status_port_A_r(Uint32 offset);
Uint32 YM2610_status_port_B_r(Uint32 offset);
Uint32 YM2610_read_port_r(Uint32 offset);
void YM2610_control_port_A_w(Uint32 offset, Uint32 data);
void YM2610_control_port_B_w(Uint32 offset, Uint32 data);
void YM2610_data_port_A_w(Uint32 offset, Uint32 data);
void YM2610_data_port_B_w(Uint32 offset, Uint32 data);

#endif
/**************** end of file ****************/
