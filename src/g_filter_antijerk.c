/*
	g_filter_antijerk.c

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

#include "g_filter_antijerk.h"

static void
g_aj_move_commit(
		struct gcode_t * g,
		gcode_pos_p pt )
{
	gcode_cmd_p cmd = g->cmds.e + pt->cmd_index;
	cmd->p |= (1 << G_AXIS_X)|(1 << G_AXIS_Y)|
			(1 << G_AXIS_E)|(1 << G_FEED);
	cmd->v[G_AXIS_X] = pt->pos.x;
	cmd->v[G_AXIS_Y] = pt->pos.y;
	cmd->v[G_AXIS_E] = pt->pos.e;
	cmd->v[G_FEED] = pt->pos.f;
}

static void
g_aj_endlayer(
		struct gcode_t * g,
		struct gcode_proc_t *p,
		uint16_t index )
{
	g_aj_p aj = (g_aj_p)p;
	gcode_layer_p layer = g->layers.e[index];

	if (layer->movecount < 2)
		return;

	if (p->debug)
		printf("%s %d\n", __func__, index);

	uint32_t lastmove = 0;
	float	lastangle = 0;
	for (int pi = layer->pt.count-1; pi >= 0; pi--) {
		gcode_pos_p pp = gcode_pos_array_get_ptr(&layer->pt, pi);

		if (pp->flags.move) {
			if (lastmove < pi) {
				lastmove = pi;
				continue;
			}
		} else
			continue;
		gcode_pos_p last = gcode_pos_array_get_ptr(&layer->pt, lastmove);

		float a = atan2(pp->pos.y - last->pos.y,
				pp->pos.x - last->pos.x) * 180 / M_PI;
		float j = fabs(a - lastangle);
		j = j > 180 ? fabs(360 - j) : j;

#if 0
		printf("%s %4d:%04d (F:%3d D:%.1fmm)/(F:%3d D:%.1fmm) diff(%.1f,%.1f) %3d\n", __func__,
			layer->index, pi,
			(int)(pp->pos.f / 60), pp->pos.d,
			(int)(last->pos.f / 60), last->pos.d,
			a, lastangle,
			(int)j);
#endif

		float newp =  (last->pos.f * 100.f) / layer->bbox.max.f;
		//float oldp =  (last->pos.f * 100.f) / g->bbox.max.f;
#if 0
		printf("SEGMENT %.2fmm at %.2f%% of max (%d/%d) angle %.2f\n",
				last->pos.d, newp,
				(int)last->pos.f/60, (int)g->bbox.max.f/60,
				j);
#endif
		// calculate a value that is a percent slowdown relative
		// to the angle of the turn. The sharper, the harder the brake
		float angleslowdown = (100 - (((int)(j/10) * 1000) / 180)) * aj->p_angleslowdownratio;
		float speedslowdown = newp *
				(aj->slow_extrude ? aj->p_extrudeslowdownratio : aj->p_speedslowdownratio);
	//	printf("Slowdown Angle %.2f Speed %.2f\n",
	//			angleslowdown, speedslowdown);

		float adjustedspeed = last->pos.f;
		adjustedspeed *= ((angleslowdown + speedslowdown) / 2) / 100;
		if (adjustedspeed < layer->bbox.min.f)
			adjustedspeed = layer->bbox.min.f;

		float splitd = last->pos.d * aj->p_splitdistanceratio;
		if (splitd > aj->p_splitdistancemax) splitd = aj->p_splitdistancemax;
		else if (splitd < aj->p_splitdistancemin) splitd = aj->p_splitdistancemin;

		if (last->pos.d <= aj->p_smallsegmentsize ||
				splitd > (last->pos.d/2) ||
				(last->pos.d - splitd <= aj->p_smallsegmentsize)) {
			if (!last->flags.extruding || (aj->slow_extrude && last->flags.extruding && !last->flags.retracting)) {
				last->pos.f = adjustedspeed;
				last->flags.touched = 1;
				last->flags.slow = 1;
				g_aj_move_commit(g, last);
			}
		} else if (!last->flags.extruding || (aj->split_extrude && last->flags.extruding && !last->flags.retracting)) {

			float splitf =  (splitd / last->pos.d);
#if 0
			printf("Splitd %.2f/%.2f ratio %f adjustedspeed %.2f\n",
					pp->pos.d,
					splitd, splitf, adjustedspeed/60);
#endif
			gcode_pos_t n = *last;
			n.pos.x += (pp->pos.x - last->pos.x) * splitf;
			n.pos.y += (pp->pos.y - last->pos.y) * splitf;
			n.pos.e += last->flags.extruding ?
					(pp->pos.e - last->pos.e) * splitf : 0;
			n.pos.d *=  splitf;
//				n.pos.f = adjustedspeed;
			n.flags.added = 1;
#if 0
			printf("NEW X:%.2f/%.2f/%.2f Y:%.2f/%.2f/%.2f E:%.2f/%.2f/%.2f F:%.2f/%.2f/%.2f\n",
					pp->pos.x, n.pos.x, last->pos.x,
					pp->pos.y, n.pos.y, last->pos.y,
					pp->pos.e, n.pos.e, last->pos.e,
					pp->pos.f, adjustedspeed, last->pos.f );
#endif
			gcode_cmd_t cmd = {
				.cmd = 'G', .cid = 1,
				.index = g->cmds.count,
				.p = (1 << G_AXIS_X)|(1 << G_AXIS_Y)|
						(1 << G_AXIS_E)|(1 << G_FEED),
				.v[G_AXIS_X] = n.pos.x,
				.v[G_AXIS_Y] = n.pos.y,
				.v[G_AXIS_E] = n.pos.e,
				.v[G_FEED] = n.pos.f,
			};
			n.cmd_index = cmd.index;
			// now adjust existing point, for niceness
			last->pos.d -= n.pos.d;
			last->pos.f = adjustedspeed;
			last->flags.touched = 1;
			g_aj_move_commit(g, pp);
			g_aj_move_commit(g, last);
			/* WARNING your points pointers will be invalid after THIS POINT */
			gcode_cmd_array_add(&g->cmds, cmd);
			gcode_pos_array_insert(&layer->pt, lastmove, &n, 1);
			pp = gcode_pos_array_get_ptr(&layer->pt, pi);
			last = gcode_pos_array_get_ptr(&layer->pt, lastmove);
			/* ^^ pfew better ^^ */
		}
		lastmove = pi;
		lastangle = a;
	}
}

g_aj_p
g_aj_register(
		gcode_p g)
{
	static const g_aj_t init = {
		.proc.name = "antijerk",
		.proc.endlayer = g_aj_endlayer,

		.split_extrude = 0,
		.slow_extrude = 1,

		.p_smallsegmentsize = 1,
		.p_speedslowdownratio	= 0.4,
		.p_angleslowdownratio = 0.5,
		.p_extrudeslowdownratio = 0.95,
		.p_splitdistanceratio = 0.95,
		.p_splitdistancemax = 2,
		.p_splitdistancemin = 0.5,
	};
	g_aj_p p = malloc(sizeof(*p));
	memcpy(p, &init, sizeof(*p));
	gcode_processor_array_add(&g->procs, &p->proc);
	return p;
}
