/*
	gcode_parser.c

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
#include "gcode.h"
#include "gcode_parser.h"

static const char * gcode_understood_params = "XYZEFBDIJGSPT";

size_t
gcode_cmd_tostring(
	gcode_cmd_p c,
	char * str,
	int maxlen,
	int line_number )
{
//	int totall = 0;
	char * base = str;

	if (line_number != -1) {
		snprintf(base, maxlen - (base - str), "N%d", line_number);
		base += strlen(base);
	}
	snprintf(base, maxlen - (base - str), "%c%d", c->cmd, c->cid);
	base += strlen(base);

	for (int i = 0; i < G_MAX; i++)
		if (c->p & (1 << i)) {
			snprintf(base, maxlen - (base - str), " %c%.4f",
				gcode_understood_params[i], c->v[i]);
			while (base[strlen(base)-1] == '0')
				base[strlen(base)-1] = 0;
			if (base[strlen(base)-1] == '.')
				base[strlen(base)-1] = 0;
			base += strlen(base);
		}
	if (line_number != -1) {
		uint8_t chk = 0;
		for (char * b = str; *b; b++)
			chk ^= *b;
		snprintf(base, maxlen - (base - str), "*%d", chk);
		base += strlen(base);
	}
	return base - str;
}

void
gcode_dumpcmd(
	const char *head,
	gcode_cmd_p c)
{
	char str[64];
	gcode_cmd_tostring(c, str, sizeof(str), -1);

	printf("%s: %s\n", head, str);
}

static int
gcode_parse_cmd(
	gcode_parser_state_p s,
	gcode_cmd_p c )
{
//	printf("CMD: '%s' (%s)\n", s->cmd, s->comment);
	char * cmd = s->cmd;
	if (toupper(*cmd) == 'N') { // skip line numbers
		int line = 0;
		while (isdigit(*cmd))
			line = (line * 10) + (*cmd++ - '0');
	}
	switch (c->cmd = *cmd++) {
		case 'G':
		case 'M':
			break;
		default:
			printf("CMD '%s': Invalid command code '%c'\n", s->cmd, c->cmd);
			return -1;
	}
	if (!isdigit(*cmd)) {
		printf("CMD '%s': Invalid command ID '%c'\n", s->cmd, *cmd);
		return -1;
	}
	while (isdigit(*cmd))
		c->cid = (c->cid * 10) + (*cmd++ - '0');
//	printf("CMD is '%c %d'\n", c->cmd, c->cid);

	while (*cmd && *cmd != '*') {
		char p = *cmd++;
		int ci = -1;
		p = toupper(p);
		for (int cif = 0; gcode_understood_params[cif] && ci == -1; cif++)
			if (p == gcode_understood_params[cif])
				ci = cif;
		float sign = 1.0f;
		switch (*cmd) {
			case '-' : sign = -1; cmd++; break;
			case '+' : sign = 1; cmd++; break;
		}

		float v = 0;
		while (isdigit(*cmd))
			v = (v * 10) + (*cmd++ - '0');
		if (*cmd == '.') {
			float f = 0.1;
			cmd++;
			while (isdigit(*cmd)) {
				v += (*cmd++ - '0') * f;
				f *= 0.1;
			}
		}
		if (ci == -1) {
			printf("CMD '%s': Unknown parameter %c%f \n", s->cmd, p, v);
			continue;
		}
	//	printf("%c%d %c%f \n", c->cmd, c->cid, p, v);
		c->p |= (1 << ci);
		c->v[ci] = v * sign;
	};
	if (*cmd == '*') {
		//checking firmware won't work anyway since we compacted the gcode
	}
//	dumpcmd(__func__, c);
	return 0;
}

int
gcode_parse_string(
	gcode_parser_state_p s,
	gcode_cmd_p ocmd,
	const char * string,
	int len)
{
	const gcode_cmd_t cz = {0};
	const char * str = string;

	while (len--) {
		char c = *str++;

		switch (c) {
			case ';': // comment
				if (!s->skipping) {
					s->commentlen = 0;
					s->skipping = 1;
					// now send command!
				} else {
					if (s->commentlen < sizeof(s->comment)-1)
						s->comment[s->commentlen++] = c;
					break;
				}
				break;
			case '\n':
			case '\r': {
				int done = 0;
				s->cmd[s->cmdlen] = 0;
				s->comment[s->commentlen] = 0;
				if (s->cmdlen > 2) {
					*ocmd = cz;
					done = gcode_parse_cmd(s, ocmd) == 0;
				}
				s->cmdlen = 0;
				s->skipping = 0;
				s->commentlen = 0;
				if (done)
					return str - string;
			}	break;
			case ' ':
			case '\t':
				if (!s->skipping || s->commentlen == 0)
					break;
			default:
				if (s->skipping) {
					if (s->commentlen < sizeof(s->comment)-1)
						s->comment[s->commentlen++] = c;
				} else {
					if (s->cmdlen < sizeof(s->cmd)-1)
						s->cmd[s->cmdlen++] = c;
				}
		}
	}
	return len;
}
