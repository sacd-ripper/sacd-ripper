#!/usr/bin/env python
# Copyright 2010 fail0verflow <master@fail0verflow.com>
# Licensed under the terms of the GNU GPL, version 2
# http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

import sys, re

tbl = open(sys.argv[1])

OPCODE_MAX = 11
types = {
		"rr": (11, "SPU_INSTR_RR", "u32 rt, u32 ra, u32 rb"),
		"rrr": (4, "SPU_INSTR_RRR", "u32 rt, u32 ra, u32 rb, u32 rc"),
		"ri7": (11, "SPU_INSTR_RI7", "u32 rt, u32 ra, u32 i7"),
		"ri8": (10, "SPU_INSTR_RI8", "u32 rt, u32 ra, u32 i8"),
		"ri10": (8, "SPU_INSTR_RI10", "u32 rt, u32 ra, u32 i10"),
		"ri16": (9, "SPU_INSTR_RI16", "u32 rt, u32 i16"),
		"ri18": (7, "SPU_INSTR_RI18", "u32 rt, u32 i18"),
		"special": (11, "SPU_INSTR_SPECIAL", "u32 opcode"),
	}

optbl = [["NULL", "SPU_INSTR_NONE"]] * (1 << OPCODE_MAX)

def decorate(f):
	return "instr_" + f

fail = False

current_instruction = None
function_prototypes = {}
function_bodies = {}
function_args = {}
function_body = None
function_attributes = {}

for line in tbl:
	if line[0] == '}':
		assert function_body is not None, "missing }"
		function_bodies[current_instruction] = function_body
		function_body = current_instruction = None
		continue
	
	if line[0] == '{':
		assert function_body is None, "missing }"
		function_body = ""
		continue
	
	if function_body is not None:
		function_body += line
		continue

	assert function_body is None, "missing }"
	
	line = line.lstrip().rstrip()
	
	if not line:
		continue

	if line[0] == '#':
		continue
	
	line = line.split(',')
	
	(l, type, args) = types[line[1]]
	opcode = int(line[0], base=2)
	opcode = opcode << (OPCODE_MAX - l)
#	print hex(opcode)
#	sys.exit(1)

	current_instruction = line[2]
	
	function_args[current_instruction] = args
	function_attributes[current_instruction] = line[3:]	
	function_bodies[current_instruction] = None

	for i in range(0, (1 << (OPCODE_MAX - l))):
		if optbl[opcode + i] != ["NULL", "SPU_INSTR_NONE"]:
			a = optbl[opcode + i]
			b = [decorate(current_instruction), type]
			print ("uh oh, would overwrite %s with %s" % (a, b))
			fail = True
		optbl = optbl[:opcode + i] + [[decorate(current_instruction), type]] + optbl[opcode + i + 1:]

instrs = ""
i = 0
for op in optbl:
	instrs = instrs + "\t{%s, %s}, // %08x\n" % (op[1], op[0], i << 25)
	i = i + 1

if fail == True:
	print instrs
	sys.exit(1)

code = ""

def print_arg(name, signed):
	if name[0] == "r":
		return '$r%d'
	elif name[0] == "i":
		return signed and '%d' or '0x%x'
	elif name == "opcode":
		return "%08x"

for fnc in function_bodies:
	args = [x.split() for  x in function_args[fnc].split(",")]
	argnames = [x[-1] for x in args]
	dump_instruction = 'vdbgprintf("%s %s\\n", %s);' % (fnc, ','.join([print_arg(x, "signed" in function_attributes[fnc]) for x in argnames]), ','.join(argnames))
	ignore_unused = ''.join("(void)%s;" % x[-1] for x in args)
	
	pre_transform = ""
	post_transform = ""

	ret = 0
	
	trap = ""
	
	for attrib in function_attributes[fnc]:
		if attrib == "signed":
			for at, an in args:
				if an[0] == "i":
					pre_transform += "%s = se%s(%s);" % (an, an[1:], an)
		elif attrib[:5] == "shift":
			for at, an in args:
				if an[0] == "i":
					pre_transform += "%s <<= %s;" % (an, attrib[5:])
		elif attrib in ["byte", "half", "Bits", "float", "double"]:
			for at, an in args:
				if an[0] == "r":
					bits = {"Bits": 1, "byte":8, "half":16, "float":32, "double":64}[attrib]
					vartype = {"Bits":"u1","byte":"u8","half":"u16","float":"float","double":"double"}[attrib]
					pre_transform += "%s %s%s[%d]; reg_to_%s(%s%s, %s);" % (vartype, an, attrib[0], 128/bits, attrib, an, attrib[0], an)
				if an == "rt":
					post_transform += "%s_to_reg(%s, %s%s);" % (attrib, an, an, attrib[0])
		elif attrib == "stop":
			ret = 1
		elif attrib == "trap":
			trap = "if (ctx->trap) return 1;"
		else:
			assert None, "Unknown attrib %s" % attrib
	
	
	if function_bodies[fnc] is None:
		ret = 1
	
	code += """int %s(%s)
{
	int stop = %d;
	/* ignore unused arguments */
	%s 
	/* pre transform */
	%s
	/* show disassembly*/
	%s
	/* optional trapping */
	%s
	/* body */
	%s
	/* post transform */
	%s
	return stop;
}
""" % (decorate(fnc), function_args[fnc], ret, ignore_unused, pre_transform, dump_instruction, trap, function_bodies[fnc] or "", post_transform)

decl = ""
for fnc in function_bodies:
	decl += "int %s(%s);\n" % (decorate(fnc), function_args[fnc])


for file in sys.argv[2:]:
	tpl = open(file + ".in").read()
	tpl = tpl.replace('###INSTRUCTIONS###', instrs)
	tpl = tpl.replace('###CODE###', code)
	tpl = tpl.replace('###DECL###', decl)
	out = open(file, "w")
	out.write(tpl)
	out.close()
	tbl.close()

