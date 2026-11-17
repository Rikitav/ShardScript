#!/usr/bin/env python3
"""Generates pathological ShardScript inputs for the parser stress suite.

Usage: generate.py <shape> <size> > case.shard
Every shape is nonsense on purpose: the parser has to report it and keep going
in bounded time rather than hang, crash or flood the diagnostics.
"""

import sys

SHAPES = {}


def shape(name):
    def register(fn):
        SHAPES[name] = fn
        return fn
    return register


@shape("junk_compilation_unit")
def junk_compilation_unit(n):
    # top-level garbage: no member ever starts, so recovery decides where to resume
    return "using stdio;\n\n" + " ".join("@#@ !!" for _ in range(n)) + "\n"


@shape("junk_statement_block")
def junk_statement_block(n):
    return header() + "func Main() -> void\n{\n" + "".join("    @ # @ ;\n" for _ in range(n)) + "}\n"


@shape("junk_struct_body")
def junk_struct_body(n):
    return "using stdio;\n\nstruct S\n{\n" + "".join("    @ # @\n" for _ in range(n)) + "}\n"


@shape("junk_no_anchor")
def junk_no_anchor(n):
    # a single junk run with no ';', '}' or member start to recover at
    return "using stdio;\n\nstruct S\n{\n" + " ".join("@#@" for _ in range(n)) + "\n"


@shape("error_lines")
def error_lines(n):
    # one broken function per line: this is the diagnostics-printing stress case
    out = ["using stdio;\n"]
    for i in range(n):
        out.append("func F%d() -> void { x := 1 + ; }\n" % i)
    return "".join(out)


@shape("unclosed_bodies")
def unclosed_bodies(n):
    return "using stdio;\n\n" + "".join(
        "func F%d() -> void\n{\n    if true\n    {\n        x := 1;\n" % i for i in range(n))


@shape("unclosed_calls")
def unclosed_calls(n):
    return header() + "func Main() -> void\n{\n    x := f(\n" + "".join(
        "    y%d := g(1, 2;\n" % i for i in range(n)) + "}\n"


@shape("stray_closers")
def stray_closers(n):
    return header() + "func Main() -> void\n{\n" + "}\n)\n]\n" * n


@shape("nested_blocks")
def nested_blocks(n):
    depth = min(n, 400)
    return (header() + "func Main() -> void\n{\n"
            + "    if true\n    {\n" * depth + "    }\n" * depth + "}\n")


@shape("nested_structs")
def nested_structs(n):
    depth = min(n, 400)
    return "".join("struct S%d\n{\n" % i for i in range(depth)) + "}\n" * depth


@shape("nested_calls")
def nested_calls(n):
    return header() + "func Main() -> void\n{\n    x := " + "f(" * n + "1" + ")" * n + ";\n}\n"


@shape("nested_generics")
def nested_generics(n):
    return header() + "func Main() -> void\n{\n    x: " + "A<" * n + "int" + ">" * n + ";\n}\n"


@shape("generic_closers")
def generic_closers(n):
    # every '>>' asks the parser to re-read one '>' : the old buffer rewrote the
    # whole remaining token list for each one
    return header() + "func Main() -> void\n{\n" + "".join(
        "    x%d := A<B<C>>;\n" % i for i in range(n)) + "}\n"


@shape("long_expression")
def long_expression(n):
    return header() + "func Main() -> void\n{\n    x := " + " + ".join("1" for _ in range(n)) + ";\n}\n"


@shape("long_identifier_chain")
def long_identifier_chain(n):
    return header() + "func Main() -> void\n{\n    x := a" + ".b" * n + ";\n}\n"


@shape("long_type_suffixes")
def long_type_suffixes(n):
    return header() + "func Main() -> void\n{\n    x: int" + "[]" * n + ";\n}\n"


@shape("long_qualified_type")
def long_qualified_type(n):
    return header() + "func Main() -> void\n{\n    x: " + "A::" * n + "B;\n}\n"


@shape("many_members")
def many_members(n):
    return "using stdio;\n\n" + "".join("func F%d() -> void { }\n" % i for i in range(n))


@shape("many_statements")
def many_statements(n):
    return header() + "func Main() -> void\n{\n" + "".join(
        "    x%d := %d;\n" % (i, i) for i in range(n)) + "}\n"


@shape("keywords_only")
def keywords_only(n):
    words = "func struct if else while for return defer try catch throw new enum interface delegate"
    return "using stdio;\n\n" + " ".join(words.split() * (n // len(words.split()) + 1)) + "\n"


def header():
    return "using stdio;\n\n"


def main():
    if len(sys.argv) < 3:
        sys.stderr.write("usage: generate.py <shape> <size>\n")
        sys.stderr.write("shapes: %s\n" % ", ".join(sorted(SHAPES)))
        return 2

    name = sys.argv[1]
    size = int(sys.argv[2])

    if name not in SHAPES:
        sys.stderr.write("unknown shape %s\n" % name)
        return 2

    sys.stdout.write(SHAPES[name](size))
    return 0


if __name__ == "__main__":
    sys.exit(main())