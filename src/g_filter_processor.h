/*
	g_filter_processor.h

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

/*
 * This filter is always the first. it detects the print head position
 * and calculates bounding boxes, statistics and so on. It also detects
 * new layer boundaries and call the other filters.
 * It will populate the gcode.layers array with all the commands and moves
 */
#ifndef __G_FILTER_PROCESSOR_H___
#define __G_FILTER_PROCESSOR_H___

#include "gcode.h"

typedef struct g_processor_t {
	gcode_proc_t proc;

	uint16_t	relative : 1;
	float	layer_thickness;
	struct	{
		int x : 1, y: 1, z : 1, e : 1, f : 1;
	}	init;
	gpoint_t pos;	// current position
	gcode_run_flags flags;
	gbox_t	bbox, bbox_e;
	float	maxretractfeed;
	float	last_e_z;	// used to detect layer changes
	float 	total_e, total_t;
	uint32_t total_moves, total_moves_e;

	/* we keep the non extruding commands around here, before
	 * moving them to the layer af the next extruding command
	 * this ensure commands at the beginning of a layer are
	 * stored on that layer
	 */
	gcode_pos_array_t pt_store;
} g_processor_t, *g_processor_p;

g_processor_p
g_processor_register(
		gcode_p g);

#endif /* __G_FILTER_PROCESSOR_H___ */
