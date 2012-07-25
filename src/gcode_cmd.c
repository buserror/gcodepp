/*
	gcode_cmd.c

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

#include <stdlib.h>
#define GCODE_CMD_PUBLIC
#include "gcode_cmd.h"
#include "c_array.h"

#define CMD_SIZE(__b)	(sizeof(gcode_cmd_t) + ((__b) * sizeof(gcode_cmd_block_t)))

gcode_cmd_p
gcode_cmd_new(
		uint8_t cmd,
		uint16_t cid)
{

}

int
gcode_cmd_has(
		gcode_cmd_p cmd,
		char param )
{
}

float
gcode_cmd_get(
		gcode_cmd_p cmd,
		char param,
		float def )
{
}

gcode_cmd_p
gcode_cmd_set(
		gcode_cmd_p cmd,
		char param,
		float val )
{
}

void
gcode_cmd_clear(
		gcode_cmd_p cmd,
		char param )
{
}
