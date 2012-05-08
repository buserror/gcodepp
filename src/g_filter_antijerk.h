/*
	g_filter_antijerk.h

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
 * This filter slows down the print head using the angle of the
 * turn and the speed of the head as factor. By default it slows
 * down all the move (ie movement without extrusion) to prevent
 * shaking of the printer and improves quality.
 * You can also have it slow down the head while extruding, and it
 * will slow down small segments and sharp corners automatically.
 */
#ifndef __G_FILTER_ANTIJERK_H___
#define __G_FILTER_ANTIJERK_H___

#include "gcode.h"

typedef struct g_aj_t {
	gcode_proc_t proc;

	int		split_extrude : 1,
			slow_extrude : 1;

	float	p_smallsegmentsize;		// in mm
	float	p_speedslowdownratio;
	float	p_extrudeslowdownratio;
	float	p_angleslowdownratio;

	float	p_splitdistanceratio;	// 0.95
	float	p_splitdistancemin;		// in mm
	float	p_splitdistancemax;		// in mm
} g_aj_t, *g_aj_p;

g_aj_p
g_aj_register(
		gcode_p g);

#endif /* __G_FILTER_ANTIJERK_H___ */
