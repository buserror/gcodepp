/*
	g_filter_cleaner.c

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

#include "g_filter_cleaner.h"


static void
g_cleaner_endlayer(
		struct gcode_t * g,
		struct gcode_proc_t *p,
		uint16_t index )
{
	g_cleaner_p c = (g_cleaner_p)p;
	gcode_layer_p layer = g->layers.e[index];

	if (p->debug)
		printf("%s %d\n", __func__, index);

	/*
	 * Walk the layer, check to see when values are duplicated
	 * in commands, and strip them out
	 */
	for (int pi = 0; pi < layer->pt.count; pi++) {
		gcode_pos_p pp = gcode_pos_array_get_ptr(&layer->pt, pi);

		if (pp->flags.move) {
			gcode_cmd_p cmd = g->cmds.e + pp->cmd_index;
			if (c->last.x == pp->pos.x) cmd->p &= ~(1 << G_AXIS_X);
			else {
				cmd->p |= (1 << G_AXIS_X);
				cmd->v[G_AXIS_X] = pp->pos.x;
			}
			if (c->last.y == pp->pos.y) cmd->p &= ~(1 << G_AXIS_Y);
			else {
				cmd->p |= (1 << G_AXIS_Y);
				cmd->v[G_AXIS_Y] = pp->pos.y;
			}
			if (c->last.z == pp->pos.z) cmd->p &= ~(1 << G_AXIS_Z);
			else {
				cmd->p |= (1 << G_AXIS_Z);
				cmd->v[G_AXIS_Z] = pp->pos.z;
			}
			if (c->last.e == pp->pos.e) cmd->p &= ~(1 << G_AXIS_E);
			else {
				cmd->p |= (1 << G_AXIS_E);
				cmd->v[G_AXIS_E] = pp->pos.e;
			}
			if (c->last.f == pp->pos.f) cmd->p &= ~(1 << G_FEED);
			else {
				cmd->p |= (1 << G_FEED);
				cmd->v[G_FEED] = pp->pos.f;
			}
		}
		c->last = pp->pos;
	}
}

g_cleaner_p
g_cleaner_register(
		gcode_p g)
{
	static const g_cleaner_t init = {
		.proc.name = "cleaner",
		.proc.endlayer = g_cleaner_endlayer,
	};
	g_cleaner_p p = malloc(sizeof(*p));
	memcpy(p, &init, sizeof(*p));
	gcode_processor_array_add(&g->procs, &p->proc);
	return p;
}

