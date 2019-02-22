#!/usr/bin/env python3
import argparse
import json

from pyparsing import Literal, Word, Group,\
    ZeroOrMore, Forward, alphas, alphanums, Regex, ParseException,\
    Suppress
import operator


class Operand(object):
    pass


class Number(Operand):
    def __init__(self, num):
        self.data = float(num)

    def json(self):
        return self.data


class Identifier(Operand):
    def __init__(self, data):
        self.data = data

    def __repr__(self):
        return self.data

    def json(self):
        return self.data


class Operation(Operand):
    def __init__(self, op, a: Operand, b: Operand):
        self.op = op
        self.a = a
        self.b = b

    def __repr__(self):
        return '{} {} {}'.format(self.a, self.op, self.b)

    def json(self):
        return {
            'operation': self.op,
            'left': self.a.json(),
            'right': self.b.json()
        }


def parse_expression(expr):
    OPERATIONS = '+', '-', '*', '/',

    for op in OPERATIONS:
        if op not in expr:
            continue
        a, b = expr.split(op, maxsplit=1)

        return Operation(op, parse_expression(a), parse_expression(b))

    return Identifier(expr)


exprStack = []


def pushFirst(strg, loc, toks):
    exprStack.append(toks[0])


def pushUMinus(strg, loc, toks):
    for t in toks:
        if t == '-':
            exprStack.append('unary -')
        else:
            break


bnf = None


def BNF():
    global bnf
    if not bnf:
        ident = Word(alphas, alphanums + '._', )

        plus, minus, mult, div = map(Literal, "+-*/")
        lpar, rpar = map(Suppress, "()")
        addop = plus | minus
        multop = mult | div

        expr = Forward()
        fnumber = Regex(r"[+-]?\d+(?:\.\d*)?(?:[eE][+-]?\d+)?")
        atom = ((ident + lpar + expr + rpar | ident | fnumber).setParseAction(pushFirst)
                | Group(lpar + expr + rpar)).setParseAction(pushUMinus)

        # by defining exponentiation as "atom [ ^ factor ]..." instead of "atom [ ^ atom ]...", we get right-to-left exponents, instead of left-to-righ
        # that is, 2^3^2 = 2^(3^2), not (2^3)^2.
        factor = Forward()
        factor << atom

        term = factor + ZeroOrMore((multop + factor).setParseAction(pushFirst))
        expr << term + ZeroOrMore((addop + term).setParseAction(pushFirst))
        bnf = expr
    return bnf


# map operator symbols to corresponding arithmetic operations
opn = {
    "+": operator.add,
    "-": operator.sub,
    "*": operator.mul,
    "/": operator.truediv}


def evaluateStack(s):
    op = s.pop()
    if op in "+-*/^":
        op2 = evaluateStack(s)
        op1 = evaluateStack(s)
        return Operation(op, op1, op2)
    elif op[0].isalpha():
        return Identifier(op)
    else:
        return Number(op)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Generate MetricQ config for arbitrary expressions')
    parser.add_argument('expression', help='The expression to be parsed')
    parser.add_argument('-o', '--output', help='The json file output path', default='config.json')

    args = parser.parse_args()

    s = args.expression

    exprStack[:] = []
    try:
        results = BNF().parseString(s, parseAll=True)
        val = evaluateStack(exprStack[:])
        print(json.dumps(val.json(), indent=4))
    except ParseException as pe:
        print(s, "failed parse:", str(pe))
    except Exception as e:
        print(s, "failed eval:", str(e))

    # print(json.dumps(parse_expression(args.expression).json(), indent=4))
