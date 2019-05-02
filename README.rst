``metricq-combinator``
======================

This tool is a MetricQ transformer that produces new, user-defined metrics
(called *combined metrics*) as arithmetic combinations of existing metrics
(called *input metrics*).

Building
--------

Build using ``cmake``::

   $ mkdir build && cd build
   $ cmake ..
   $ make

Usage
-----

Basic setup (MetricQ management server, token etc) is configured via command
line; for more information, issue::

   $ metricq-combinator --help

The actual information on how to combine new metrics is provided as a JSON
object by the management server, mapping the names of metrics-to-be-combined to
their configuration::

   {
      "metrics": {
         <name of combined metric>: {
            "expression": <expression>,
            "metadata": { ... }
         },
         ...
      }
   }

For each combined metric, the key ``"expression"`` is mandatory and describes
this metric as a tree of binary operations and input metrics/constants::

   <expression> ::= "<input metric>"
   <expression> ::= <constant value (float/integer)>
   <expression> ::= {
                        "left": <expression>,
                        "right": <expression>,
                        "operation": ("+" | "-" | "*" | "/")
                    }

The key ``"metadata"`` is optional and maps to a JSON object containing
arbitrary metadata for this combined metric.  These are sent to the manager when
declaring the new metric.  Commonly used metadata-keys are:

``"unit"``
   A string containing the SI unit symbol in which this metric reports values.

``"description"``
   A string containing the human-readable description of what this metric
   reports.

``"display_expression"``
   A string containing a human-readable version of this metrics
   ``"expression"``-key, i.e. a short description on how this metric is derived
   from its inputs, in infix-notation.

Examples
--------

* Report the total power consumption of two systems *foo* and *bar* as a new
  metric ``foobar.power``::

    "foobar.power": {
       "expression": {
          "left":      "foo.power",
          "operation": "+"
          "right":     "bar.power",
       },
       "metadata": {
          "unit": "W",
          "description": "Total power consumption of foo and bar",
          "display_expression": "foo.power + bar.power",
       }
    }

* Provide a temperature reading in a different unit::

    "RoomA15.temperature.imperial": {
      "expression": {
        "left": {
           "left": "RoomA15.temperature",
           "right": 1.8,
           "operation": "*"
        },
        "right": 32,
        "operation": "+"
      },
      "metadata": {
        "unit": "°F",
        "description": "Room A15 temperature reading, in degrees Fahrenheit"
        "display_expression": "(RoomA15.temperature * 9/5) + 32"
      }
    }

License
-------

::

  Combine new MetricQ metrics in a flexible way
  Copyright (C) 2019  Technische Universität Dresden

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.
