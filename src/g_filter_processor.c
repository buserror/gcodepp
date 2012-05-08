/*
	g_filter_processor.c

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
#include <string.h>
#include "g_filter_processor.h"

#define GET_DELTA(__g, __v, __current) ((__g)->relative ? (__v) : ((__v) - (__current)))
#define GET_ABSOLUTE(__g, __v, __current) ((__g)->relative ? ((__v) + (__current)) : (__v))

float gpoint_distance(gpoint_p a, gpoint_p b)
{
	float dx = a->x - b->x;
	float dy = a->y - b->y;
	float dz = a->z - b->z;

	return sqrtf((dx*dx)+(dy*dy)+(dz*dz));
}
void gbox_add(gbox_p b, gpoint_p p)
{
#define cmpv(b, p, C)\
	if (b->max.C == 0 && b->max.C == b->min.C)	b->max.C = b->min.C = p->C;\
	else if (p->C > b->max.C)	b->max.C = p->C;\
	else if (p->C < b->min.C)	b->min.C = p->C;
	cmpv(b, p, x);
	cmpv(b, p, y);
	cmpv(b, p, z);
	cmpv(b, p, e);
	cmpv(b, p, f);
#undef cmpv
}

/*
 * Really store a point to the layer
 */
static int
_gcode_store_pt(
		gcode_p g,
		g_processor_p p,
		gcode_pos_p pt)
{
	if (!g) return -1;
	if (!g->layer)
		gcode_new_layer(g);
	gcode_pos_array_add(&g->layer->pt, *pt);

	gbox_add(&p->bbox, &pt->pos);
	gbox_add(&g->layer->bbox, &pt->pos);

	if (pt->flags.move) {
		if (pt->flags.extruding) {
			gbox_add(&p->bbox_e, &pt->pos);
			gbox_add(&g->layer->bbox_e, &pt->pos);
		}
		g->layer->movecount++;
		G_PROCESSOR_CALL_CHAIN(g, move, g->layer->pt.e + (g->layer->pt.count-1));
	}

	return 0;
}

/*
 * This flushed the temporary store of commands once an extruding
 * move has been detected. This is made so that moves and positioning
 * at the start of a layer are stored with that layer, not on the
 * previous one.
 */
int
_gcode_store_flush(
		gcode_p g,
		g_processor_p p)
{
	if (!p->pt_store.count)
		return 0;
	if (!g->layer)
		gcode_new_layer(g);
	for (int fi = 0; fi < p->pt_store.count; fi++)
		_gcode_store_pt(g, p, p->pt_store.e + fi);
	gcode_pos_array_clear(&p->pt_store);
	return 0;
}

/*
 * Store the current position and flags to the current layer
 * (with a twist)
 */
static int
gcode_store_pt(
		gcode_p g,
		g_processor_p p,
		gcode_cmd_p c)
{
	if (!g) return -1;
	if (!g->layer)
		gcode_new_layer(g);
	gcode_pos_t pt = {
		.cmd_index = c->index,
		.pos = p->pos,
		.flags = p->flags,
	};
	if (pt.flags.extruding) {
		_gcode_store_flush(g, p);
		_gcode_store_pt(g, p, &pt);
	} else
		gcode_pos_array_add(&p->pt_store, pt);

	return 0;
}

static int
_gcode_process_command_g(
	gcode_p g,
	g_processor_p p,
	gcode_cmd_p c )
{
	gpoint_t start = p->pos;

	switch (c->cid) {
		case 2:		// ARC X/Y/E I/J
		case 3:		// ARC X/Y/E I/J
		case 0:
		case 1: {		// Move
			p->total_moves++;
			p->flags.move = 1;

			if (c->p & (1 << G_AXIS_Z)) {
				if (!p->init.z) {
					p->init.z = 1;
					p->pos.z = c->v[G_AXIS_Z];
					p->bbox.min.z = p->bbox.max.z = p->pos.z; // start there
				//	printf("Layer %4d assume origin is %f\n", p->layer, p->pos.z);
				}
				p->pos.z = GET_ABSOLUTE(p, c->v[G_AXIS_Z], p->pos.z);
			//	printf("Layer %4d Z moves to %f\n", g->layer, g->pos.z);
			}
			if (c->p & (1 << G_AXIS_X)) {
				if (!p->init.x) {
					p->init.x = 1;
					p->pos.x = c->v[G_AXIS_X];
				}
				p->pos.x = GET_ABSOLUTE(p, c->v[G_AXIS_X], p->pos.x);
			}
			if (c->p & (1 << G_AXIS_Y)) {
				if (!p->init.y) {
					p->init.y = 1;
					p->pos.y = c->v[G_AXIS_Y];
				}
				p->pos.y = GET_ABSOLUTE(p, c->v[G_AXIS_Y], p->pos.y);
			}
			if (c->p & (1 << G_FEED)) {
			//	printf("Layer %4d feed change from %f to %f\n", g->layer, g->feed, c->v[G_FEED]);
			//	printf("Layer %4d total=%f feed=%f\n", g->layer, p->total_t, p->feed);
			//	if (c->v[G_FEED] == 0)
			//		gcode_dumpcmd("ZERO FEED ??", c);
				p->pos.f = c->v[G_FEED];
				if (p->pos.f < start.f)
					start.f = p->pos.f; //(feed - p->feed) / 2;
			}
			if (c->p & (1 << G_AXIS_E)) {
				float delta = GET_DELTA(p, c->v[G_AXIS_E], p->pos.e);
				p->pos.e = GET_ABSOLUTE(p, c->v[G_AXIS_E], p->pos.e);
				if (delta > 0) {
					p->flags.extruding = 1;
					p->total_moves_e++;
					p->total_e += delta;
					// This is a new layer
					if (p->last_e_z != p->pos.z) {
						if (g->layers.count < 2) {
							p->layer_thickness = p->pos.z - p->last_e_z;
							printf("Layer %4d assume layer thickness is %f\n",
									g->layers.count, p->layer_thickness);
						}
						p->last_e_z = p->pos.z;
						gcode_new_layer(g);
					}
				} else if (delta < 0) {
					p->flags.extruding = 1;
					p->flags.retracting = 1;
					p->maxretractfeed = start.f > p->maxretractfeed ? start.f : p->maxretractfeed;
				}
			}
			if (start.f > 0) {
				float dis = gpoint_distance(&start, &p->pos);
				p->pos.d = dis;
				float tim = dis * (1 / (start.f / 60.0f));
				p->total_t += tim;
			}
		}	break;
		case 90:	// use absolute coordinates
			p->relative = 0;
			break;
		case 91:	// use relative coordinates
			p->relative = 1;
			break;
		case 21:	// set units to millimeters
			break;
		case 92: {	// set origins
			if (c->p & (1 << G_AXIS_X)) {
				p->init.x = 1;
				p->pos.x = c->v[G_AXIS_X];
				printf("Layer %4d X origin set to %f\n", g->layers.count, p->pos.x);
			}
			if (c->p & (1 << G_AXIS_Y)) {
				p->init.y = 1;
				p->pos.y = c->v[G_AXIS_Y];
				printf("Layer %4d Y origin set to %f\n", g->layers.count, p->pos.z);
			}
			if (c->p & (1 << G_AXIS_Z)) {
				p->init.z = 1;
				p->pos.z = c->v[G_AXIS_Z];
				printf("Layer %4d Z origin set to %f\n", g->layers.count, p->pos.z);
			}
			if (c->p & (1 << G_AXIS_E)) {
				p->init.e = 1;
				p->pos.e = c->v[G_AXIS_E];
			//	printf("Layer %4d E origin set to %f\n", g->layer, p->pos.e);
			}
		}	break;
		default:
			gcode_dumpcmd(__func__, c);
			break;
	}
	return 0;
}

static int
_gcode_process_command_m(
	gcode_p g,
	g_processor_p p,
	gcode_cmd_p c )
{
	switch (c->cid) {
		case 17: 	// Motors ON
			break;
		case 18: 	// Motors OFF
			break;
		case 84:	// all motors off
			break;
		case 105:	// report temperatures
			break;
		case 106:	// Fan ON S<0-255>
		case 107:	// Fan Off
			break;
		case 109: 	// wait for temperature to be reached S=<celcius>
			break;
		case 114:	// report current position
			break;
		case 119:	// Marlin: report endstop status
			break;
		case 190:	// set bed temperature S<celcius>
			break;
		case 104:	// set temperature S<celcius>
			break;
		case 140:	// set bed temperature
			break;
		case 82:	// use absolute distances for extrusion
			break;
		default:
			gcode_dumpcmd(__func__, c);
			break;
	}
	return 0;
}

static int
g_processor_command(
	gcode_p g,
	gcode_proc_p _p,
	gcode_cmd_p c )
{
	g_processor_p p = (g_processor_p)_p;
	int res = -1;
	p->flags.v = 0;
	p->pos.d = 0;
	switch (c->cmd) {
		case 'G':
			res = _gcode_process_command_g(g, p, c);
			break;
		case 'M':
			res = _gcode_process_command_m(g, p, c);
			break;
		default:
			res = -1;
	}
	//if (!res)
	gcode_store_pt(g, p, c);

	return res;
}

void
_g_processor_newlayer(
		struct gcode_t * g,
		struct gcode_proc_t *_p,
		uint16_t index )
{
	g_processor_p p = (g_processor_p)_p;
	g->layer->z = p->pos.z;
}

static void
g_processor_finish(
	gcode_p g,
	gcode_proc_p _p)
{
	g_processor_p p = (g_processor_p)_p;
	_gcode_store_flush(g, p);	// add remaining non-extruding commands

	printf("Max F E:%.2f (%.2fmm/s)\n",
			p->bbox.max.f, p->bbox.max.f / 60);
	printf("Max F Extrude E:%.2f (%.2fmm/s)\n",
			p->bbox_e.max.f, p->bbox_e.max.f / 60);
	printf("Max F Retract E:%.2f (%.2fmm/s)\n",
			p->maxretractfeed, p->maxretractfeed/60);
	printf("Total moves %d extruded points %d\n",
		(int)p->total_moves, (int)p->total_moves_e);
	printf("Total %d layers %.02fmm total E %.02fmm Time %2d:%02d:%02d\n",
		g->layers.count, p->bbox.max.z, p->total_e,
		(int)(p->total_t / 60 / 60),
		(int)(p->total_t / 60) % 60, (int)p->total_t % 60);
}
g_processor_p
g_processor_register(
		gcode_p g)
{
	static const g_processor_t init = {
		.proc.name = "processor",
		.proc.command = g_processor_command,
		.proc.finish = g_processor_finish,
	};
	g_processor_p p = malloc(sizeof(*p));
	memcpy(p, &init, sizeof(*p));
	gcode_processor_array_add(&g->procs, &p->proc);
	return p;
}

