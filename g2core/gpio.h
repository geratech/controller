/*
* gpio.h - Digital IO  handling functions
* This file is part of the g2core project
*
* Copyright (c) 2015 - 2017 Alden S. Hart, Jr.
* Copyright (c) 2015 - 2017 Robert Giseburt
*
* This file ("the software") is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License, version 2 as published by the
* Free Software Foundation. You should have received a copy of the GNU General Public
* License, version 2 along with the software.  If not, see <http://www.gnu.org/licenses/>.
*
* As a special exception, you may use this file as part of a software library without
* restriction. Specifically, if other files instantiate templates or use macros or
* inline functions from this file, or you compile this file and link it with  other
* files to produce an executable, this file does not by itself cause the resulting
* executable to be covered by the GNU General Public License. This exception does not
* however invalidate any other reasons why the executable file might be covered by the
* GNU General Public License.
*
* THE SOFTWARE IS DISTRIBUTED IN THE HOPE THAT IT WILL BE USEFUL, BUT WITHOUT ANY
* WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
* OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT
* SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
* OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef GPIO_H_ONCE
#define GPIO_H_ONCE

#include "dynamic_registry.h"

#define INPUT_LOCKOUT_MS 10          // milliseconds to go dead after input firing
#define D_OUT_CHANNELS 9
#define A_IN_CHANNELS 0
#define A_OUT_CHANNELS 0
//--- do not change from here down ---//

typedef enum {
	IO_ACTIVE_LOW = 0,                  // input is active low (aka normally open)
	IO_ACTIVE_HIGH = 1,                 // input is active high (aka normally closed)
	IO_MODE_DISABLED = 2,               // input is disabled
	IO_MODE_MAX                         // unused. Just for range checking
} ioMode;
#define NORMALLY_OPEN   IO_ACTIVE_LOW   // equivalent
#define NORMALLY_CLOSED IO_ACTIVE_HIGH  // equivalent

typedef enum {                          // actions are initiated from within the input's ISR
	INPUT_ACTION_NONE = 0,
	INPUT_ACTION_STOP,                  // stop at normal jerk - preserves positional accuracy
	INPUT_ACTION_FAST_STOP,             // stop at high jerk - preserves positional accuracy
	INPUT_ACTION_HALT,                  // stop immediately - not guaranteed to preserve position
	INPUT_ACTION_CYCLE_START,           // start / restart cycle after feedhold (RESERVED)
	INPUT_ACTION_ALARM,                 // initiate an alarm. stops everything immediately - preserves position
	INPUT_ACTION_SHUTDOWN,              // initiate a shutdown. stops everything immediately - does not preserve position
	INPUT_ACTION_PANIC,                 // initiate a panic. stops everything immediately - does not preserve position
	INPUT_ACTION_RESET,                 // reset system
	INPUT_ACTION_MAX                    // unused. Just for range checking
} inputAction;

typedef enum {                          // functions are requested from the ISR, run from the main loop
	INPUT_FUNCTION_NONE = 0,
	INPUT_FUNCTION_LIMIT = 1,           // limit switch processing
	INPUT_FUNCTION_INTERLOCK = 2,       // interlock processing
	INPUT_FUNCTION_SHUTDOWN = 3,        // shutdown in support of external emergency stop
	INPUT_FUNCTION_PROBE = 4,           // assign input as probe input
	INPUT_FUNCTION_TOOL_HEIGHT_SETTER = 5,
	INPUT_FUNCTION_MAX                  // unused. Just for range checking
} inputFunc;

typedef enum {
	INPUT_INACTIVE = 0,                 // aka switch open, also read as 'false'
	INPUT_ACTIVE = 1,                   // aka switch closed, also read as 'true'
	INPUT_DISABLED = 2                  // value returned if input is disabled
} ioState;

typedef enum {
	INPUT_EDGE_NONE = 0,                // no edge detected or edge flag reset (must be zero)
	INPUT_EDGE_LEADING,                 // flag is set when leading edge is detected
	INPUT_EDGE_TRAILING                 // flag is set when trailing edge is detected
} inputEdgeFlag;

/*
* GPIO structures
*/

class ioDigitalInput {		              // one struct per digital input
public:
	ioState state;                      // input state 0=inactive, 1=active, -1=disabled
	inputEdgeFlag edge;                 // keeps a transient record of edges for immediate inquiry
	bool homing_mode;                   // set true when input is in homing mode.
	bool probing_mode;                  // set true when input is in probing mode.
	Motate::Timeout lockout_timer;      // time to expire current debounce lockout, or 0 if no lockout
	
	bool trueBool = true;
	DynamicRegistry<bool*>::EntryType safteyInterlockEntry = {&trueBool, nullptr};

	uint16_t external_pin_num;
	
	void updateValue (bool inputValue); // Update the input's value
	
	//inputAction getAction () { return this->action; }
	//void setAction (inputAction _action) { action = _action; }
	//inputFunc getFunction () { return this->function;}
	//void setFunction (inputFunc _function) {
		//this->function = _function;
	//}
	
	ioMode mode;					              // -1=disabled, 0=active low (NO), 1= active high (NC)
	inputAction action;                 // 0=none, 1=stop, 2=halt, 3=stop_steps, 4=reset
	inputFunc function;                 // function to perform when activated / deactivated
	uint16_t lockout_ms;                // number of milliseconds for debounce lockout
	
	void reset(void);                   // Reset the input
	
	ioDigitalInput (uint16_t _ext_pin_num, ioMode _mode = IO_MODE_DISABLED, inputAction _action = INPUT_ACTION_NONE, inputFunc _function = INPUT_FUNCTION_NONE, uint16_t _lockout_ms = INPUT_LOCKOUT_MS) {
		this->mode = _mode;
		this->action = _action;
		this->function = _function;
		this->lockout_ms = _lockout_ms;
		this->external_pin_num = _ext_pin_num;
		this->reset();
	}
};

typedef ioDigitalInput d_in_t;

typedef struct gpioDigitalOutput {      // one struct per digital output
	ioMode mode;
} d_out_t;

typedef struct gpioAnalogInput {        // one struct per analog input
	ioMode mode;
} a_in_t;

typedef struct gpioAnalogOutput {       // one struct per analog output
	ioMode mode;
} a_out_t;

extern DynamicRegistry<ioDigitalInput*> digitalInputs;
extern d_out_t  d_out[D_OUT_CHANNELS];
extern a_in_t   a_in[A_IN_CHANNELS];
extern a_out_t  a_out[A_OUT_CHANNELS];

/*
* GPIO function prototypes
*/

void gpio_init(void);
void gpio_reset(void);
void input_reset(void);
void output_reset(void);

bool gpio_read_input(const uint8_t input_num);
void gpio_set_homing_mode(const uint8_t input_num, const bool is_homing);
void gpio_set_probing_mode(const uint8_t input_num, const bool is_probing);
int8_t gpio_get_probing_input(void);

stat_t io_set_mo(nvObj_t *nv);
stat_t io_set_ac(nvObj_t *nv);
stat_t io_set_fn(nvObj_t *nv);

stat_t io_get_input(nvObj_t *nv);

stat_t io_set_domode(nvObj_t *nv);			// output sense
stat_t io_get_output(nvObj_t *nv);
stat_t io_set_output(nvObj_t *nv);

#ifdef __TEXT_MODE
void io_print_mo(nvObj_t *nv);
void io_print_ac(nvObj_t *nv);
void io_print_fn(nvObj_t *nv);
void io_print_in(nvObj_t *nv);
void io_print_domode(nvObj_t *nv);
void io_print_out(nvObj_t *nv);
#else
#define io_print_mo tx_print_stub
#define io_print_ac tx_print_stub
#define io_print_fn tx_print_stub
#define io_print_in tx_print_stub
#define io_print_st tx_print_stub
#define io_print_domode tx_print_stub
#define io_print_out tx_print_stub
#endif // __TEXT_MODE

#endif // End of include guard: GPIO_H_ONCE
