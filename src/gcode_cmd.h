/*
	gcode_cmd.h

	Copyright 2008-2012 Michel Pollet <buserror@gmail.com>

 	This file is part of simavr.

	simavr is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	simavr is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with simavr.  If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef __GCODE_CMD_H___
#define __GCODE_CMD_H___

#define GCODE_CMD_PUBLIC

typedef uint32_t gcode_cmd_index_t;

#ifdef GCODE_CMD_PUBLIC

typedef uint32_t gcode_cmd_block_t;

#define _GCODE_CMD_BLOCK_SIZE (sizeof(gcode_cmd_block_t))

typedef struct gcode_cmd_block_t {
	gcode_cmd_block_t has;
	float	v[_GCODE_CMD_BLOCK_SIZE];
} gcode_cmd_block_t;

typedef struct gcode_cmd_t {
	uint32_t 	cmd : 8,			// "G", "M" etc
				cid : 16,			// GXXX, MXXX etc
				_b_count : 4;
	gcode_cmd_index_t	index;		// command id number
	gcode_cmd_block_t	b[0];
} gcode_cmd_t, *gcode_cmd_p;

#else
typedef void * gcode_cmd_p;
#endif

gcode_cmd_p
gcode_cmd_new(
		uint8_t cmd,
		uint16_t cid);

int
gcode_cmd_has(
		gcode_cmd_p cmd,
		char param );
float
gcode_cmd_get(
		gcode_cmd_p cmd,
		char param,
		float def );

gcode_cmd_p
gcode_cmd_set(
		gcode_cmd_p cmd,
		char param,
		float val );

void
gcode_cmd_clear(
		gcode_cmd_p cmd,
		char param );

#endif /* __GCODE_CMD_H___ */
