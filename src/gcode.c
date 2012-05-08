/*
	gcode.c

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
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "gcode.h"
#include "gcode_parser.h"


static const gcode_cmd_t gcode_cmd_zero = {0};


void
gcode_init(
		gcode_p g)
{
	memset(g, 0, sizeof(*g));
}


/*
 * Finish the current layer
 */
int
gcode_finish_layer(
		gcode_p g )
{
	if (!g || !g->layer) return -1;
	G_PROCESSOR_CALL_CHAIN(g, endlayer, g->layer->index);
	return 0;
}

/*
 * Allocate a new layer
 */
int
gcode_new_layer(
		gcode_p g )
{
	if (!g) return -1;
	if (g->layer)
		gcode_finish_layer(g);
	gcode_layer_t l = {
		.index = g->layers.count,
		/* remaining are filled by filters */
	};
	gcode_layer_p p = malloc(sizeof(*p));
	*p = l;
	gcode_layer_array_add(&g->layers, p);
	g->layer = p;
	G_PROCESSOR_CALL_CHAIN(g, newlayer, g->layer->index);
	return 0;
}

int
gcode_read(
	gcode_p g,
	const char * fname)
{
	gcode_parser_state_t s = {0};
	int fd = open(fname, O_RDONLY);
	if (fd == -1) {
		perror(fname);
		return -1;
	}

	do {
		char buffer[1024];
		ssize_t rd = read(fd, buffer, sizeof(buffer));
		if (rd == -1) {
			perror(fname);
			close(fd);
			return -1;
		}
		if (rd == 0)
			break;
	//	printf("read %d\n", (int)rd);
		int done = 0;
		do {
			gcode_cmd_t c;
			int r = gcode_parse_string(&s, &c, buffer + done, rd - done);
			done += r;
		//	printf("rd = %d/%d = %d\n", done, rd, r);
			if (r <= 0) break;

			// store it first in the global array
			c.index = g->cmds.count;
			gcode_cmd_array_add(&g->cmds, c);
			// then process it. We do not pass the one in the array,
			// as filters might change the array underneath it
			G_PROCESSOR_CALL_CHAIN(g, command, &c);
		} while (done < rd);
	} while (1);
	gcode_finish_layer(g);
	close(fd);
	G_PROCESSOR_CALL_CHAIN(g, finish);

	return 0;
}
