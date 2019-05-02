#!/usr/bin/env python
import sys
import json
import logging

import click
import click_completion
import click_log

click_completion.init()

logger = logging.getLogger(__name__)

click_log.basic_config(logger)
logger.setLevel('WARNING')
logger.handlers[0].formatter = logging.Formatter(
    fmt='%(asctime)s [%(levelname)-8s] [%(name)-20s] %(message)s')


from pyparsing import Literal, Word, \
    ZeroOrMore, alphas, alphanums, nums, \
    Suppress, infixNotation, opAssoc, oneOf


class Operand(object):
    def __init__(self):
        pass


class Counter(Operand):
    def __init__(self, path):
        self.path = list(path)

    def __repr__(self):
        return 'Counter("{}")'.format(self.path)

    def __str__(self):
        return ".".join(self.path)

    def json(self):
        return self.__str__()


class Constant(Operand):
    def __init__(self, value):
        self.value = float(value)

    def __str__(self):
        return f'{self.value:.12g}'

    def __repr__(self):
        return "Constant({0.value})".format(self)

    def json(self):
        return self.value


class Combined(Operand):
    def __init__(self, left, op, right):
        self.left = left
        self.op = op
        self.right = right

    def __repr__(self):
        return f"Combined(left: {self.left}, op: {self.op}, right: {self.right})"

    def __str__(self):
        return f"({self.left} {self.op} {self.right})"

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
            return Combined(left, op, right)

    if not isinstance(combined, Combined):
        return combined

    if combined.op not in "+*":
        return combined

    logger.debug("combined: {}".format(combined))
    exprs = gather_same_ops(combined)
    logger.debug("gathered: {}".format(exprs))
    exprs = list(map(balance_combined_expression, exprs))
    logger.debug("recursive: {}".format(exprs))
    balanced = make_balanced(exprs, combined.op)
    logger.debug("balanced: {}".format(balanced))
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

        COUNTER_GRAMMAR = (ZeroOrMore(part + sep) +
                           part).setParseAction(Counter)
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
        const = Word(nums).setParseAction(lambda tok: Constant(tok[0]))

        expr = infixNotation(counter | const, [
            (oneOf(['+', '-', '*', '/']), 2, opAssoc.RIGHT,
             lambda tok: Combined(tok[0][0], tok[0][1], tok[0][2])),
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


def translate_metadata(config):
    metadata = dict()
    for key in ('description', 'unit'):
        if key in config:
            metadata[key] = config[key]

    if 'altunit' in config and 'multiplier' in config:
        metadata['altunit'] = {
            'name': config['altunit'],
            'multiplier': config['multiplier']
        }

    return metadata


@click.command()
@click.argument('legacy_config', type=click.File('rb'), default=sys.stdin)
@click.option('-b',
              '--balance',
              is_flag=True,
              default=False,
              help='balance complex expression trees')
@click_log.simple_verbosity_option(logger)
def translate(legacy_config, balance):
    """Generate configurations for combined metrics from the legacy "Dataheap"-format

    LEGACY_CONFIG:  a JSON file describing legacy counters
    """

    legacy_config = json.load(legacy_config)

    new_metrics = dict()
    for counter, config in legacy_config['counters'].items():
        derived = config.get('derived')
        if derived is None:
            continue

        logger.debug("Translating counter '{}'...".format(counter))
        name = str(parse_with(legacy_counter_grammar(), counter))
        expression = parse_with(legacy_derived_grammar(), derived)

        if balance:
            expression = balance_combined_expression(expression)

        metadata = translate_metadata(config)
        metadata['display_expression'] = str(expression)

        new_metrics[name] = {
            'expression': expression.json(),
            'metadata': metadata,
        }

    for chunk in OperandEncoder().iterencode({'metrics': new_metrics}):
        sys.stdout.write(chunk)


if __name__ == '__main__':
    translate()
