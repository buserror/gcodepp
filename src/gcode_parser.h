/*
	gcode_parser.h

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

#ifndef __GCODE_PARSER_H__
#define __GCODE_PARSER_H__

/*
 * This is the gcode parser status... The cmd and comment
 * Are populated and will stay in text form there after a
 * call to gcode_parse_string()
 */
typedef struct gcode_parser_state_t {
	int skipping;
	char cmd[128];
	int cmdlen;
	char comment[256];
	int commentlen;
} gcode_parser_state_t, *gcode_parser_state_p;

/*
 * Parses a string 'str' up to 'len' byte long, decodes and
 * digest one command and return it in 'ocmd'.
 */
int
gcode_parse_string(
	gcode_parser_state_p s,
	gcode_cmd_p ocmd,
	const char * str,
	int len);

/*
 * converts a digested command back to text. If line_number is -1,
 * no checksum is generated. Otherwise, one is added.
 */
size_t
gcode_cmd_tostring(
	gcode_cmd_p c,
	char * str,
	int maxlen,
	int line_number /* = -1 */);


#endif // __GCODE_PARSER_H__
