/*
	gcode.h

	Copyright 2012 Michel Pollet <buserror@gmail.com>

 	This file is part of gcodepp.

	gcodepp is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	gcodepp is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with gcodepp.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __GCODE_H__
#define __GCODE_H__

#include <stdint.h>
#include <string.h>
#include "c_array.h"

/*
 * List of understood parameters
 */
enum {
	G_AXIS_X, G_AXIS_Y, G_AXIS_Z, G_AXIS_E, G_FEED, 
	G_B, G_D, G_I, G_J, G_G, G_S, G_P, G_T,
	G_MAX
};

/*
 * "digested" gcode command
 */
typedef struct gcode_cmd_t {
	uint32_t 	cmd : 8,	// "G", "M" etc
				cid : 16;	// GXXX, MXXX etc
	uint32_t	index;		// command index number
	uint16_t 	p;			// bitmask for the present parameters
	float 		v[G_MAX];	// parameters
} gcode_cmd_t, *gcode_cmd_p;

#if G_MAX > 15
#error the bitmask 'p' needs a bigger type
#endif

typedef struct gpoint_t {
	float x,y,z,e,f,d;
} gpoint_t, *gpoint_p;

typedef struct gbox_t {
	gpoint_t min, max;
} gbox_t, *gbox_p;

struct gcode_layer_t;
DECLARE_C_ARRAY(struct gcode_layer_t *, gcode_layer_array, 8);
DECLARE_C_ARRAY(gcode_cmd_t, gcode_cmd_array, 32);

typedef union {
	struct {
		uint32_t move : 1, extruding: 1, retracting : 1,
				slow : 1,
				added : 1, /* for commands we have inserted */
				touched : 1, /* for commands we tweaked */
				/* todo */
				skin : 1, infill : 1,
				bridge : 1, overhang : 1;
	};
	uint32_t v;
} gcode_run_flags;

/*
 * This links the current "virtual position" of the 'head' with
 * the gcode command index, and also some higher level flags
 * that indicate what the command is.
 */
typedef struct gcode_pos_t {
	uint32_t cmd_index;
	gcode_run_flags flags;
	gpoint_t pos;
} gcode_pos_t, *gcode_pos_p;

DECLARE_C_ARRAY(gcode_pos_t, gcode_pos_array, 32);

struct gcode_t;
struct gcode_proc_t;

typedef int (*gcode_command_callback_t)(
		struct gcode_t * g,
		struct gcode_proc_t *p,
		gcode_cmd_p cmd );
typedef void (*gcode_newlayer_callback_t)(
		struct gcode_t * g,
		struct gcode_proc_t *p,
		uint16_t index );
typedef void (*gcode_endlayer_callback_t)(
		struct gcode_t * g,
		struct gcode_proc_t *p,
		uint16_t index );
typedef void (*gcode_move_callback_t)(
		struct gcode_t * g,
		struct gcode_proc_t *p,
		gcode_pos_p pt );
typedef void (*gcode_finish_callback_t)(
		struct gcode_t * g,
		struct gcode_proc_t *p);
typedef int (*gcode_argument_callback_t)(
		struct gcode_t * g,
		struct gcode_proc_t *p,
		const char ** argv, int argc );

typedef struct gcode_proc_t {
	const char name[32];
	uint32_t debug : 1;
	gcode_command_callback_t command;
	gcode_newlayer_callback_t newlayer;
	gcode_endlayer_callback_t endlayer;
	gcode_move_callback_t move;
	gcode_finish_callback_t finish;
	gcode_argument_callback_t argument;
} gcode_proc_t, *gcode_proc_p;

#define G_PROCESSOR_CALL_CHAIN(__g, __callback, __args...)\
{	for (int _pi = 0; _pi < __g->procs.count; _pi++) {\
		gcode_proc_p _ite = __g->procs.e[_pi];\
		if ((_ite)->__callback)\
			(_ite)->__callback(__g, _ite, ##__args); \
	}\
}

DECLARE_C_ARRAY(gcode_proc_p, gcode_processor_array, 4);

/*
 * Main gcode anchor structure
 * This contains a series of filters, and will end up
 * with a populated list of layers, and a list of
 * commands corresponding.
 */
typedef struct gcode_t {
	/* array of filter to call */
	gcode_processor_array_t	procs;

	/* global array of commands */
	gcode_cmd_array_t	cmds;

	/* global array of layers */
	gcode_layer_array_t layers;
	struct gcode_layer_t * layer;	// current layer

} gcode_t, *gcode_p;

/*
 *
 */
typedef struct gcode_layer_t {
	int index;
	/* These are filled by g_filter_processor */
	float z;
	gbox_t	bbox, bbox_e;
	uint32_t	movecount;	// not necessarily accurate, after cmds are added
	gcode_pos_array_t pt;
} gcode_layer_t, *gcode_layer_p;

/*
 * Zeroes a gcode structure
 */
void
gcode_init(
		gcode_p g);

/* TODO: Cleanup calls */

/*
 * Reads a file, parses it, sends it to the filters
 */
int
gcode_read(
	gcode_p g,
	const char * fname);

/*
 * Internal calls for g_filter_processor - TODO: move away
 */
int
gcode_new_layer(
		gcode_p g );
int
gcode_finish_layer(
		gcode_p g );

/*
 * dump the digested command in it's text form
 */
void gcode_dumpcmd(
	const char *head,
	gcode_cmd_p c);

IMPLEMENT_C_ARRAY(gcode_layer_array);
IMPLEMENT_C_ARRAY(gcode_cmd_array);
IMPLEMENT_C_ARRAY(gcode_pos_array);
IMPLEMENT_C_ARRAY(gcode_processor_array);

#endif // __GCODE_H__
