#!/usr/bin/env python3
# Copyright (c) 2019, ZIH, Technische Universitaet Dresden, Federal Republic of Germany
#
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification,
# are permitted provided that the following conditions are met:
#
#     * Redistributions of source code must retain the above copyright notice,
#       this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright notice,
#       this list of conditions and the following disclaimer in the documentation
#       and/or other materials provided with the distribution.
#     * Neither the name of metricq nor the names of its contributors
#       may be used to endorse or promote products derived from this software
#       without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

import asyncio
import logging
import pprint
import time
import sys
import re
import json

import click
import click_completion
import click_log

import metricq

logger = metricq.get_logger()

click_log.basic_config(logger)
logger.setLevel('WARNING')
logger.handlers[0].formatter = logging.Formatter(
    fmt='%(asctime)s [%(levelname)-8s] [%(name)-20s] %(message)s')

click_completion.init()


async def aget_missing_metrics(server, token, metrics):
    """Retrieve the subset of metrics that are not found on a given MetricQ
    server

    :param server:  MetricQ server URL
    :param token:   MetricQ client token
    :param metrics: iterable containing the metrics to check

    :returns: A set() containing the names of all metrics that are not found on
              the server
    """
    metrics = list(metrics)
    client = metricq.HistoryClient(token=token,
                                   management_url=server,
                                   event_loop=asyncio.get_running_loop())
    await client.connect()

    async def get_matches(metric):
        selector = re.escape(metric)
        match = await client.history_metric_list(selector)
        logger.debug(f'Matching metrics for selector "{selector}": {match}')
        return match

    matches = await asyncio.gather(*map(get_matches, metrics))
    logger.debug(f'Metrics: {metrics}')
    logger.debug(f'Matches: {matches}')

    missing = set(metric for metric, match in zip(metrics, matches)
                  if not match)

    await client.stop()

    return missing


def get_inputs(optree):
    if isinstance(optree, str):
        return set((optree, ))
    elif isinstance(optree, dict):
        return get_inputs(optree['left']) | get_inputs(optree['right'])
    elif isinstance(optree, (float, int)):
        return set()
    else:
        raise TypeError(f"Unexpected operand type: {optree:r}")


def get_all_inputs(metrics):
    """Return the set of metrics used as inputs across a collection of combined
    metrics
    """
    inputs = set()
    for metric, optree in metrics.items():
        logger.info(f"Checking inputs for {metric}...")
        this_inputs = get_inputs(optree)
        logger.info(f"Inputs for {metric}: {this_inputs}")
        inputs |= this_inputs

    return inputs


def pretty_print_missing(missing):
    def echo(msg):
        click.echo(click.style(msg, fg='bright_blue'))

    echo("Missing metrics:")
    for metric in missing:
        echo(f"* {metric}")


PRINTERS = {
    'json': lambda missing: json.dump(list(missing), sys.stdout),
    'pretty': pretty_print_missing
}


@click.command()
@click.option('--server', default='amqp://localhost/')
@click.option('--token', default='metric-availability-checker')
@click.option('-p',
              '--printer',
              default='pretty',
              type=click.Choice(PRINTERS.keys()))
@click.argument('config_file', type=click.File('rb'), default=sys.stdin)
@click_log.simple_verbosity_option(logger)
def check_availability(server, token, printer, config_file):
    """List metrics from a combinator configuration file that are neither \
declared by itself, nor a MetricQ server.

    The configuration file format is

    \b
      { "metrics": [ "<combined metric>": <optree>, ...] }

    where <optree> is recursively defined as

    \b
      <optree> ::= <numeric constant>
                 | "<input metric>"
                 | { "left": <optree>, "right": <optree>, "operation": "..." }
    """
    config = json.load(config_file)

    if not isinstance(config, dict) or 'metrics' not in config:
        raise click.BadArgumentUsage(
            f"{config_file.name} must be a JSON dictionary containing a 'metrics' key"
        )

    metrics = config['metrics']
    inputs = get_all_inputs(metrics)

    # The configuration file might use metrics as inputs that it declares
    # itself.  Filter out these metrics, as we are only interested in metrics
    # that are both missing from a server and are not combined by ourselves.
    inputs = filter(lambda m: m not in metrics, inputs)

    missing = asyncio.run(aget_missing_metrics(server, token, inputs))

    PRINTERS[printer](missing)


if __name__ == '__main__':
    check_availability()
