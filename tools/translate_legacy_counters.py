#!/usr/bin/env python
import sys
import json
import argparse

from pyparsing import Literal, Word, \
    ZeroOrMore, alphas, alphanums, nums, \
    Suppress, infixNotation, opAssoc, oneOf


def dbg(msg):
    print(":: ", msg, file=sys.stderr)


class Operand(object):
    def __init__(self):
        pass


class Counter(Operand):
    def __init__(self, path):
        self.path = [part.replace('-', '_') for part in path]

    def __repr__(self):
        return 'Counter("{}")'.format(self.path)

    def __str__(self):
        return ".".join(self.path)

    def json(self):
        return self.__str__()


class Constant(Operand):
    def __init__(self, value):
        self.value = float(value[0])

    def __str__(self):
        return str(self.value)

    def __repr__(self):
        return "Constant({0.value})".format(self)

    def json(self):
        return self.value


class Combined(Operand):
    def __init__(self, toks):
        self.left, self.op, self.right = toks[0]

    def __repr__(self):
        return "Combined(left: {0.left}, op: {0.op}, right: {0.right})".format(
            self)

    def __str__(self):
        return "({0.left} {0.op} {0.right})".format(self)

    def json(self):
        return {
            "left": self.left.json(),
            "right": self.right.json(),
            "operation": self.op
        }


def balance_combined_expression(combined: Combined):
    def gather_same_ops(comb, op=None):
        if isinstance(comb, Combined):
            if op is None:
                op = comb.op
            if comb.op == op:
                return [comb.left] + gather_same_ops(comb.right, op)
            else:
                return [comb]
        elif isinstance(comb, (Counter, Constant)):
            return [comb]
        else:
            return []

    def make_balanced(metrics, op):
        nmetrics = len(metrics)
        if nmetrics == 0:
            return []
        elif nmetrics == 1:
            return metrics[0]
        else:
            middle = nmetrics // 2
            left = make_balanced(metrics[:middle], op)
            right = make_balanced(metrics[middle:], op)
            return Combined([[left, op, right]])

    if not isinstance(combined, Combined):
        return combined

    if combined.op not in "+*":
        return combined

    dbg("combined: {}".format(combined))
    exprs = gather_same_ops(combined)
    dbg("gathered: {}".format(exprs))
    exprs = list(map(balance_combined_expression, exprs))
    dbg("recursive: {}".format(exprs))
    balanced = make_balanced(exprs, combined.op)
    dbg("balanced: {}".format(balanced))
    return balanced


class OperandEncoder(json.JSONEncoder):
    def default(self, obj):
        if isinstance(obj, Operand):
            return obj.json()
        else:
            return json.JSONEncoder.default(self, o)


COUNTER_GRAMMAR = None


def legacy_counter_grammar():
    '''Get the grammar describing how to parse the name of a counter.

    EBNF:
        counter ::= (part '/')* part
        part    ::= alphas (alphanums | '-' | '_')
    '''
    global COUNTER_GRAMMAR
    if not COUNTER_GRAMMAR:
        sep = Suppress('/')
        part = Word(
            alphas,
            alphanums + '_-',
        )

        COUNTER_GRAMMAR = (
            ZeroOrMore(part + sep) + part).setParseAction(Counter)
    return COUNTER_GRAMMAR


GRAMMAR = None


def legacy_derived_grammar():
    '''Get the grammar describing the expression tree for a Combined metric.

    EBNF:
        Combined ::= expr
        expr    ::= '(' expr ')' | '{' counter '}' | (expr op expr) | const
        op      ::= ('+' | '-' | '*' | '/')
        const   ::= [0-9]+

    For a description of 'counter', see legacy_counter_grammar.
    '''
    global GRAMMAR
    if not GRAMMAR:
        lbr, rbr = map(Suppress, "{}")
        counter = lbr + legacy_counter_grammar() + rbr
        const = Word(nums).setParseAction(Constant)

        expr = infixNotation(counter | const, [
            (oneOf(['+', '-', '*', '/']), 2, opAssoc.RIGHT, Combined),
        ])

        # expr.setDebug().runTests('''
        #     {a/b/foo}
        #     42
        #     {bar}+{baz}
        #     2*{frob}*({bar}+{baz})
        #     {a}+{b}+{c}+{d}
        #     ''')

        expr.enablePackrat()

        GRAMMAR = expr
    return GRAMMAR


def parse_with(parser, s):
    return parser.parseString(s, parseAll=True)[0]


def test_balance():
    c = parse_with(legacy_derived_grammar(), "{foo}+{bar}+{baz}+(2*{frob})+1")
    balance_combined_expression(c)


def translate(configs, balance=False):
    new_metrics = dict()
    for counter, config in configs['counters'].items():
        derived = config.get('derived')
        if derived is None:
            continue
        dbg("Translating counter '{}'...".format(counter))
        new_name = parse_with(legacy_counter_grammar(), counter)
        new_config = parse_with(legacy_derived_grammar(), derived)
        if balance:
            new_config = balance_combined_expression(new_config)
        new_metrics[str(new_name)] = new_config
    return {'metrics': new_metrics}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description=
        'Generate configurations for combined metrics from the legacy "Dataheap"-format'
    )
    parser.add_argument(
        'infile',
        metavar="LEGACY_CONFIG",
        help="Path to a config file (JSON) describing legacy counters",
        type=argparse.FileType(),
        default=sys.stdin)
    parser.add_argument(
        '-b',
        '--balance',
        help='balance complex expression trees',
        action='store_true')

    args = parser.parse_args()

    new_config = translate(json.load(args.infile), args.balance)
    for chunk in OperandEncoder().iterencode(new_config):
        sys.stdout.write(chunk)
