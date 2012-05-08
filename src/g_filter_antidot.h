/*
	g_filter_antidot.h

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
 * This filter detects "extrusion dots" often left by Slic3r
 * Basicaly a single large move that goes and extrude a tiny
 * dot and then moves off are just removed from the output
 */
#ifndef __G_FILTER_ANTIDOT_H___
#define __G_FILTER_ANTIDOT_H___

#include "gcode.h"

typedef struct g_ad_t {
	gcode_proc_t proc;
	int totaldots;
} g_ad_t, *g_ad_p;

g_ad_p
g_antidot_register(
		gcode_p g);

#endif /* __G_FILTER_ANTIDOT_H___ */
