/*
	gcoderead.c

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

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>

#include "gcode.h"
#include "g_filter_processor.h"
#include "g_filter_antijerk.h"
#include "g_filter_cleaner.h"
#include "g_filter_antidot.h"
#include "gcode_parser.h"


char *
gcode_flags_get_comment(
		uint32_t oldf,
		uint32_t newf,
		char * str,
		size_t max)
{
	//gcode_run_flags old = { .v = oldf };
	gcode_run_flags new = { .v = newf };
	* str = 0;
//	sprintf(str, "%03x", newf);
	if (new.retracting)
		strcat(str, " Retract");
	else if (new.extruding)
		strcat(str, " Extrude");
	else if (new.move)
		strcat(str, " Move");
	if (new.slow)
		strcat(str, " Slow");
	if (new.added)
		strcat(str, " NEW");
	if (new.touched)
		strcat(str, " Touched");
	return str;
}

static void usage(int e)
{
	fprintf(stderr,
			"gcodepp: <gcode file>\n"
			"\t-o <filename>:\n"
			"\t--output <filename>: gcode output file\n"
			"\t-vo\n"
			"\t--verbose-output: verbose gcode output\n"
			"\t-d\n"
			"\t--debug: debug traces\n"
	);
	exit(e);
}

int
main(
		int argc,
		const char * argv[])
{
	const char * output = NULL;
	const char * input = NULL;
	int verbose = 0;
	int output_verbose = 0;
	gcode_t code;
	gcode_t * g = &code;	// for cut/paste laziness

	gcode_init(g);

	g_processor_p g_g = g_processor_register(g);	// main gcode interpreter
	g_ad_p		g_antidot = g_antidot_register(g);
	g_aj_p		g_antijerk = g_aj_register(g);
	g_cleaner_p g_cleaner = g_cleaner_register(g);

	for (int i = 1; i < argc; i++) {
		/*
		 * Pass arguments to filters first
		 */
		{
			int skip = 0;
			for (int _pi = 0; _pi < g->procs.count && !skip; _pi++) {
				gcode_proc_p _ite = g->procs.e[_pi];
				if ((_ite)->argument)\
					skip = (_ite)->argument(g, _ite, &argv[i], argc-i);
			}
			if (skip > 0) {
				i += skip - 1;
				continue;
			}
			if (skip < 0)
				usage(1);
		}

		if (!strcmp(argv[i], "-o") || !strcmp(argv[i], "--output"))
			output = argv[++i];
		else if (!strcmp(argv[i], "-d") || !strcmp(argv[i], "--debug"))
			verbose++;
		else if (!strcmp(argv[i], "-vo") || !strcmp(argv[i], "--verbose-output"))
			output_verbose++;
		else if (argv[i][0] == '-') {
			fprintf(stderr, "%s: Unknown parameter '%s'\n", argv[0], argv[i]);
			usage(1);
		} else
			input = argv[i];
	}
	if (!input)
		usage(1);

	gcode_read(g, input);


	if (output) {
		printf("Output file is %s\n", output);
		FILE * out = stdout;
		if (strcmp(output, "-")) {
			out = fopen(output, "w");
			if (!out)
				perror(output);
		}
		uint32_t lasti = -1;
		uint32_t lastf = -1;

		for (int li = 0; li < g->layers.count; li++) {
			gcode_layer_p layer = g->layers.e[li];
			if (output_verbose)
				fprintf(out, "; Layer %d\n", layer->index);
			for (int ci = 0; ci < layer->pt.count; ci++) {
				if (layer->pt.e[ci].cmd_index != lasti) {
					lasti = layer->pt.e[ci].cmd_index;
					char str[64], comment[128];
					gcode_cmd_tostring(&g->cmds.e[lasti], str, sizeof(str), -1);
					fprintf(out, "%s", str);
					if (output_verbose && lastf != layer->pt.e[ci].flags.v) {
						gcode_flags_get_comment(lastf, layer->pt.e[ci].flags.v,
								comment, sizeof(comment));
						lastf = layer->pt.e[ci].flags.v;
						fprintf(out, " ; %s", comment);
					}
					fprintf(out, "\n");
				}
			}
		}
		if (out != stdout)
			fclose(out);
	}
}
