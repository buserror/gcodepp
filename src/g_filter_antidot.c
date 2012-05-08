/*
	g_filter_antidot.c

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

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "g_filter_antidot.h"

DECLARE_C_ARRAY(uint32_t, index_array, 8);
IMPLEMENT_C_ARRAY(index_array);

/*
 * This is what we search and destroy
 * G92 E0 ;
	G1 X95.754 Y102.817 F8400 ;  Move
	G1 E1.25 F1500 ;  Extrude
	G1 X95.248 Y102.311 E1.2601 F4200
	G1 E0.0101 F1500 ;  Retract
	G92 E0 ;
 *
 */
static void
g_antidot_endlayer(
		struct gcode_t * g,
		struct gcode_proc_t *p,
		uint16_t index )
{
	g_ad_p c = (g_ad_p)p;
	gcode_layer_p layer = g->layers.e[index];
	index_array_t kill = C_ARRAY_NULL;

	if (p->debug)
		printf("%s %d Z:%.2f\n", __func__, index, layer->z);

	/*
	 * Walk the layer, check to see if we find a "dot" of extrusion
	 * like Slice3r does at version <= 0.72+ and strip it out entirely
	 */
	struct {
		gcode_pos_p p;
		gcode_cmd_p cmd;
	} window[6];
	for (int pi = 0; pi < layer->pt.count; pi++) {
		gcode_pos_p pp = gcode_pos_array_get_ptr(&layer->pt, pi);

		memmove(&window[1],&window[0], 5*sizeof(window[0]));
		window[0].p = pp;
		window[0].cmd = g->cmds.e + pp->cmd_index;

		if (pi < 6)
			continue;
		if (
				window[0].cmd->cmd == 'G' && window[0].cmd->cid == 92 &&
				(window[0].cmd->p & (1 << G_AXIS_E)) && window[0].cmd->v[G_AXIS_E] == 0 &&
				window[1].p->flags.retracting &&
				window[2].p->flags.extruding && window[2].p->pos.d <= 0.8 &&
				window[4].p->flags.move && !window[4].p->flags.extruding &&
				window[5].cmd->cmd == 'G' && window[5].cmd->cid == 92 &&
				(window[5].cmd->p & (1 << G_AXIS_E)) && window[5].cmd->v[G_AXIS_E] == 0
			) {
#if 0
			printf("Found a dot index %d\n", pi-4);
			for (int ci = 5; ci >= 0; ci--) {
				char str[64];
				gcode_cmd_tostring(window[ci].cmd, str, sizeof(str), -1);
				printf("> %s (d=%.2f)\n", str, window[ci].p->pos.d);
			}
#endif
			index_array_add(&kill, pi - 4);
		}
	}
	if (kill.count) {
		if (p->debug)
			printf("%s deleting %d dots\n", __func__, kill.count);
		for (int ki = kill.count-1; ki >= 0; ki--) {
			if (p->debug)
				printf("%s deleting dot at index %d\n", __func__, kill.e[ki]);
			gcode_pos_array_delete(&layer->pt, kill.e[ki], 5);
			c->totaldots++;
		}
	}
	index_array_free(&kill);
}

static void
g_antidot_finish(
		struct gcode_t * g,
		struct gcode_proc_t *p )
{
	g_ad_p c = (g_ad_p)p;
	printf("%s removed %d dots\n", p->name, c->totaldots);
}

g_ad_p
g_antidot_register(
		gcode_p g)
{
	static const g_ad_t init = {
		.proc.name = "antidot",
		.proc.endlayer = g_antidot_endlayer,
		.proc.finish = g_antidot_finish,
	};
	g_ad_p p = malloc(sizeof(*p));
	memcpy(p, &init, sizeof(*p));
	gcode_processor_array_add(&g->procs, &p->proc);
	return p;
}
