#ifndef _VMT35LIB_H
#define _VMT35LIB_H 1

#include "configCosmos.h"

/**
* Contains definitions and functions for running a VMT-35
* Torque Rod controller.
* @file vmt35_lib.h
* @brief VMT35 controller includes
*/

//! \ingroup devices
//!	\defgroup vmt35 VMT-35 Magnetic Torque Rod Controller
//! Vectronic Aerospace Magnetic Torque Rod control library

//#include <stdio.h>
//#include <errno.h>

#include "mathlib.h"
#include "cssl_lib.h"
#include "cosmos-errno.h"

#define VMT35_ID 1
#define VMT35_BAUD 115200
#define VMT35_BITS 8
#define VMT35_PARITY 0
#define VMT35_STOPBITS 1


/**
* Internal structure containing the state of the VMT35.
* @brief VMT35 state
*/
typedef struct
	{
	uint16_t status;
	uint16_t count;
	uint16_t invalidcount;
	int32_t dac[3];
	uint16_t temp;
	uint16_t voltage;
	uint8_t resetcount;
	uint8_t crc;
	} vmt35_telemetry;

typedef struct
{
	cssl_t *serial;
	vmt35_telemetry telem;
} vmt35_handle;

int32_t vmt35_connect(char *dev, vmt35_handle *handle);
int32_t vmt35_disconnect(vmt35_handle *handle);
int32_t vmt35_reset(vmt35_handle *handle);
int32_t vmt35_set_voltage(vmt35_handle *handle, uint8_t channel, uint16_t voltage);
int32_t vmt35_set_percent_voltage(vmt35_handle *handle, int16_t ch, int16_t percentage);
int32_t vmt35_set_current_dac(vmt35_handle *handle, uint8_t channel, uint16_t current);
int32_t vmt35_set_current(vmt35_handle *handle, uint8_t channel, float current);
int32_t vmt35_enable(vmt35_handle *handle);
int32_t vmt35_disable(vmt35_handle *handle);
int32_t vmt35_reverse(vmt35_handle *handle, uint8_t channel);
int32_t vmt35_get_telemetry(vmt35_handle *handle);
int32_t vmt35_get_voltage(vmt35_handle *handle, uint8_t channel, uint16_t *voltage);
int32_t vmt35_get_current(vmt35_handle *handle, uint8_t channel, int16_t *current);
int32_t vmt35_set_amps(vmt35_handle *handle, uint8_t channel,float current);
int32_t vmt35_set_moment(vmt35_handle *handle, uint16_t channel, double mom, float npoly[7], float ppoly[7]);
int32_t vmt35_set_moments(vmt35_handle *handle, rvector mom, float npoly[3][7], float ppoly[3][7]);
int32_t vmt35_putbyte(vmt35_handle *handle, uint8_t byte);
uint8_t vmt35_crc(uint8_t *string, uint16_t length);
double vmt35_calc_amp(double mom, float npoly[6], float ppoly[6]);
double vmt35_calc_moment(double amp, float npoly[6], float ppoly[6]);

#endif
