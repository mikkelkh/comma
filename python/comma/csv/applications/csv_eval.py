# This file is part of comma, a generic and flexible library
# Copyright (c) 2011 The University of Sydney
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. Neither the name of the University of Sydney nor the
#    names of its contributors may be used to endorse or promote products
#    derived from this software without specific prior written permission.
#
# NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
# GRANTED BY THIS LICENSE.  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
# HOLDERS AND CONTRIBUTORS \"AS IS\" AND ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
# BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
# OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
# IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

import sys
import os
import argparse
import numpy as np
import re
import itertools
import ast
import comma

description = """
evaluate numerical expressions and append computed values to csv stream
"""


notes_and_examples = """
input fields:
    1) slashes are replaced by underscores if --full-xpath is given, otherwise basenames are used
    2) for ascii streams, input fields are treated as floating point numbers, unless --format is given

output fields:
    1) inferred from expressions (by default) or specified by --output-fields
    2) appended to possibly modified input
    3) treated as floating point numbers, unless --output-format is given

examples:
    # basic
    ( echo 1; echo 2; echo 3 ) | %(prog)s --fields=x 'y = x**2'

    # using an intermediate variable
    ( echo 1; echo 2; echo 3 ) | %(prog)s --fields=x 'n = 2; y = x**n' --output-fields=y

    # ascii stream with non-default formats
    ( echo 0,1; echo 1,1 ) | %(prog)s --fields=x,y 'n = x<y' --output-format=ub
    ( echo 0,1; echo 1,1 ) | %(prog)s --fields=i,j --format=2ub 'n = i==j' --output-format=ub

    # binary stream
    ( echo 0.1,2; echo 0.1,3 ) | csv-to-bin d,i | %(prog)s --binary=d,i --fields=x,n 'y = x**n' | csv-from-bin d,i,d

    # evaluate one of two expressions based on condition
    ( echo 1,2; echo 2,1 ) | %(prog)s --fields=x,y 'a=where(x<y,x+y,x-y)'

    # select output based on condition
    ( echo 1,2 ; echo 1,3; echo 1,4 ) | %(prog)s --fields=a,b --format=2i --select="(a < b - 1) & (b < 4)"

    # pass through input until condition is met
    ( echo 1,2 ; echo 1,3; echo 1,4 ) | csv-eval --fields=a,b --format=2i --exit-if="(a < b - 1) & (b < 4)"
    
    # update input stream values in place
    ( echo 1,2 ; echo 3,4 ) | %(prog)s --fields=x,y "x=x+y; y=y-1"

    # using full xpaths
    ( echo 1,2 ; echo 3,4 ) | %(prog)s --fields=one/x,two/y "x+=1; y-=1"
    ( echo 1,2 ; echo 3,4 ) | %(prog)s --fields=one/x,two/y "one_x+=1; two_y-=1" --full-xpath

    # using default values
    ( echo 1,2 ; echo 3,4 ) | %(prog)s --fields=,y "a=x+y" --default-values="x=0;y=0"
"""

numpy_functions = """
functions:
    any function documented at
    http://docs.scipy.org/doc/numpy/reference/routines.html
    can be used in expressions provided that it is compatible with streaming, that is:
        - it performs element-wise operations only
        - it returns an array of the same shape and size as the input
    some examples are given below

math functions:
    http://docs.scipy.org/doc/numpy/reference/routines.math.html

    ( echo 1,2; echo 3,4 ) | %(prog)s --fields=x,y --precision=2 'a = 2/(x+y); b = a*sin(x-y)'
    ( echo 1,2; echo 4,3 ) | %(prog)s --fields=x,y 'm = minimum(x,y)'
    ( echo 1; echo 2; echo 3; echo 4 ) | %(prog)s --format=ui --fields=id 'c = clip(id,3,inf)' --output-format=ui

math constants: pi, e
    echo pi | %(prog)s --fields name --format=s[2] 'value=pi' --precision=16 --delimiter='='
    echo e | %(prog)s --fields name --format=s[2] 'value=e' --precision=16 --delimiter='='

logical functions:
    http://docs.scipy.org/doc/numpy/reference/routines.logic.html

    ( echo 0,1; echo 1,2; echo 4,3 ) | %(prog)s --fields=x,y 'flag=logical_and(x<y,y<2)' --output-format=b
    ( echo 0,1; echo 1,2; echo 4,3 ) | %(prog)s --fields=x,y 'flag=logical_or(x>y,y<2)' --output-format=b
    ( echo 0; echo 1 ) | %(prog)s --format=b --fields=flag 'a=logical_not(flag)' --output-format=b

bitwise functions:
    http://docs.scipy.org/doc/numpy/reference/routines.bitwise.html

    ( echo 0; echo 1 ) | %(prog)s --fields i --format=ub 'n = ~i'
    ( echo 0; echo 1 ) | %(prog)s --fields i --format=ub 'n = ~i.astype(bool)'
    ( echo 0,0; echo 0,1; echo 1,1 ) | %(prog)s --fields i,j --format=2ub 'm = i & j'
    ( echo 0,0; echo 0,1; echo 1,1 ) | %(prog)s --fields i,j --format=2ub 'm = i | j'

string functions:
    http://docs.scipy.org/doc/numpy/reference/routines.char.html

    ( echo 'a'; echo 'a/b' ) | %(prog)s --fields=path --format=s[36] 'n=char.count(path,"/")' --output-format=ui
    ( echo 'a'; echo 'a/b' ) | %(prog)s --fields=path --format=s[36] 'r=char.replace(path,"/","_")' --output-format=s[36]

time arithmetic:
    http://docs.scipy.org/doc/numpy/reference/arrays.datetime.html#datetime-and-timedelta-arithmetic

    echo 20150101T000000.000000 | %(prog)s --fields=t --format=t 't1=t+1;t2=t-1' --output-format=2t
    echo 20151231T000000 | %(prog)s --fields=t --format=t "t += timedelta64(1,'D')"
    echo 20151231T000000,20160515T120000 | %(prog)s --fields=t1,t2 --format=2t "dt = (t2-t1)/timedelta64(1,'D')"
"""


class csv_eval_error(Exception):
    pass


def custom_formatwarning(msg, *args):
    return __name__ + " warning: " + str(msg) + '\n'


def add_csv_options(parser):
    comma.csv.add_options(parser, defaults={'fields': 'x,y,z'})
    parser.add_argument(
        '--format',
        default='',
        metavar='<format>',
        help="for ascii stream, format of named input fields (by default, 'd' for each)")
    parser.add_argument(
        '--output-fields',
        '-o',
        default=None,
        metavar='<names>',
        help="do not infer output fields from expressions; use specified fields instead")
    parser.add_argument(
        '--output-format',
        default='',
        metavar='<format>',
        help="format of output fields (default: 'd' for each)")
    # the options defined below are left for compatibility
    # use --output-fields and --output-format instead
    parser.add_argument('--append-fields', '-F', help=argparse.SUPPRESS)
    parser.add_argument('--append-binary', '-B', help=argparse.SUPPRESS)


def get_args():
    parser = argparse.ArgumentParser(
        description=description,
        epilog=notes_and_examples,
        formatter_class=comma.util.argparse_fmt,
        add_help=False)
    parser.add_argument(
        'expressions',
        help='numerical expressions to evaluate (see examples)',
        nargs='?')
    parser.add_argument(
        '--help',
        '-h',
        action='store_true',
        help='show this help message and exit')
    parser.add_argument(
        '--verbose',
        '-v',
        action='store_true',
        help='more output to stderr')
    parser.add_argument(
        '--permissive',
        action='store_true',
        help='leave python builtins in the exec environment (use with care)')
    add_csv_options(parser)
    parser.add_argument(
        '--full-xpath',
        action='store_true',
        help='use full xpaths as variable names with / replaced by _ (default: use basenames of xpaths)')
    parser.add_argument(
        '--default-values',
        '--default',
        default='',
        metavar='<assignments>',
        help='default values for variables in expressions but not in input stream')
    parser.add_argument(
        '--with-error',
        default='',
        metavar='<message>',
        help='if --exit-if, exit with error and a given error message')
    parser.add_argument(
        '--exit-if',
        '--output-until',
        '--until',
        default='',
        metavar='<condition>',
        help='output all records and exit when the condition is satisfied')
    parser.add_argument(
        '--select',
        '--output-if',
        '--if',
        default='',
        metavar='<condition>',
        help='select and output records of input stream that satisfy the condition')
    args = parser.parse_args()
    if args.help:
        if args.verbose:
            parser.epilog += numpy_functions
        else:
            parser.epilog += "\nfor more help run '%(prog)s -h -v'"
        parser.print_help()
        parser.exit(0)
    return args


def ingest_deprecated_options(args):
    if args.append_binary:
        args.output_format = args.append_binary
        del args.append_binary
        if args.verbose:
            with comma.util.warning(custom_formatwarning) as warn:
                msg = "--append-binary is deprecated, consider using --output-format"
                warn(msg)
    if args.append_fields:
        args.output_fields = args.append_fields
        del args.append_fields
        if args.verbose:
            with comma.util.warning(custom_formatwarning) as warn:
                msg = "--append-fields is deprecated, consider using --output-fields"
                warn(msg)


def check_options(args):
    if not (args.expressions or args.select or args.exit_if):
        raise csv_eval_error("no expressions are given")
    if args.binary and args.format:
        raise csv_eval_error("--binary and --format are mutually exclusive")
    if args.select or args.exit_if:
        if args.expressions:
            msg = "--select <condition> and --exit-if <condition> cannot be used with expressions"
            raise csv_eval_error(msg)
        if args.output_fields or args.output_format:
            msg = "--select and --exit-if cannot be used with --output-fields or --output-format"
            raise csv_eval_error(msg)
    if args.with_error:
        if not args.exit_if:
            msg = "--with-error is only used with --exit-if"
            raise csv_eval_error(msg)


def format_without_blanks(format, fields=[], unnamed_fields=True):
    """
    >>> from comma.csv.applications.csv_eval import format_without_blanks
    >>> format_without_blanks('3ui', fields=['a', 'b', 'c'])
    'ui,ui,ui'
    >>> format_without_blanks('ui', fields=['a', 'b', 'c'])
    'ui,d,d'
    >>> format_without_blanks('ui', fields=['a', '', 'c'])
    'ui,s[0],d'
    >>> format_without_blanks('4ui', fields=['a', '', 'c'])
    'ui,s[0],ui,s[0]'
    >>> format_without_blanks('3ui')
    's[0],s[0],s[0]'
    >>> format_without_blanks('')
    ''
    >>> format_without_blanks('ui,t', ['a', 'b'], unnamed_fields=False)
    'ui,t'
    >>> format_without_blanks('ui,t', ['a', 'b', 'c'], unnamed_fields=False)
    'ui,t,d'
    >>> format_without_blanks('ui,,t', ['a', 'b', 'c'], unnamed_fields=False)
    'ui,d,t'
    >>> format_without_blanks('ui,t', ['', 'b'], unnamed_fields=False)
    Traceback (most recent call last):
     ...
    ValueError: expected all fields to be named, got ',b'
    >>> format_without_blanks('ui,t,d', ['a', 'b'], unnamed_fields=False)
    Traceback (most recent call last):
     ...
    ValueError: format 'ui,t,d' is longer than fields 'a,b'
    """
    def comma_type(maybe_type, field, default_type='d', type_of_unnamed_field='s[0]'):
        return type_of_unnamed_field if not field else maybe_type or default_type

    if not format and not fields:
        return ''
    maybe_types = comma.csv.format.expand(format).split(',')
    if not unnamed_fields:
        if '' in fields:
            msg = "expected all fields to be named, got '{}'".format(','.join(fields))
            raise ValueError(msg)
        if len(maybe_types) > len(fields):
            msg = "format '{}' is longer than fields '{}'".format(format, ','.join(fields))
            raise ValueError(msg)
    maybe_typed_fields = itertools.izip_longest(maybe_types, fields)
    types = [comma_type(maybe_type, field) for maybe_type, field in maybe_typed_fields]
    return ','.join(types)


def assignment_variable_names(expressions):
    """
    >>> from comma.csv.applications.csv_eval import assignment_variable_names
    >>> assignment_variable_names("a = 1; b = x + y; c = 'x = 1; y = 2'; d = (b == z)")
    ['a', 'b', 'c', 'd']
    >>> assignment_variable_names("a, b = 1, 2")
    ['a', 'b']
    >>> assignment_variable_names("a = b = 1")
    ['a', 'b']
    >>> assignment_variable_names("x = 'a = \\"y = 1;a = 2\\"';")
    ['x']
    >>> assignment_variable_names("")
    []
    >>> assignment_variable_names("x=1; x=2; y+=1; y+=2; z=1; z+=2")
    ['x', 'y', 'z']
    """
    tree = ast.parse(expressions, '<string>', mode='exec')
    fields = []
    for child in ast.iter_child_nodes(tree):
        if type(child) == ast.Assign:
            for target in child.targets:
                for node in ast.walk(target):
                    if type(node) == ast.Name:
                        if node.id not in fields:
                            fields.append(node.id)
        elif type(child) == ast.AugAssign:
            if child.target.id not in fields:
                fields.append(child.target.id)
    return fields


def split_fields(fields):
    """
    >>> from comma.csv.applications.csv_eval import split_fields
    >>> split_fields('')
    []
    >>> split_fields('x')
    ['x']
    >>> split_fields('x,y')
    ['x', 'y']
    """
    return fields.split(',') if fields else []


def normalise_full_xpath(fields, full_xpath=True):
    """
    >>> from comma.csv.applications.csv_eval import normalise_full_xpath
    >>> normalise_full_xpath('')
    []
    >>> normalise_full_xpath('a/b')
    ['a_b']
    >>> normalise_full_xpath(',a/b,,c,d/e,')
    ['', 'a_b', '', 'c', 'd_e', '']
    >>> normalise_full_xpath(',a/b,,c,d/e,', full_xpath=False)
    ['', 'b', '', 'c', 'e', '']
    """
    full_xpath_fields = split_fields(fields)
    if full_xpath:
        return [f.replace('/', '_') for f in full_xpath_fields]
    return [f.split('/')[-1] for f in full_xpath_fields]


def prepare_options(args):
    ingest_deprecated_options(args)
    check_options(args)
    check_fields(assignment_variable_names(args.default_values))
    args.fields = normalise_full_xpath(args.fields, args.full_xpath)
    if args.binary:
        args.format = comma.csv.format.expand(args.binary)
        args.binary = True
    else:
        args.format = format_without_blanks(args.format, args.fields)
        args.binary = False
    if args.select or args.exit_if:
        return
    var_names = assignment_variable_names(args.expressions)
    args.update_fields = [f for f in var_names if f in args.fields]
    if args.output_fields is None:
        args.output_fields = [f for f in var_names if f not in args.fields]
    else:
        args.output_fields = split_fields(args.output_fields)
    args.output_format = format_without_blanks(args.output_format,
                                               args.output_fields,
                                               unnamed_fields=False)


def restricted_numpy_env():
    d = np.__dict__.copy()
    d.update(__builtins__={})
    d.pop('sys', None)
    return d


def update_buffer(stream, update_array):
    index = stream.fields.index
    if stream.binary:
        fields = stream._input_array.dtype.names
        for f in update_array.dtype.names:
            stream._input_array[fields[index(f)]] = update_array[f]
    else:
        def updated_lines():
            for line, scalars in itertools.izip(stream._ascii_buffer, update_array):
                values = line.split(stream.delimiter)
                for f, s in zip(update_array.dtype.names, stream._strings(scalars)):
                    values[index(f)] = s
                yield stream.delimiter.join(values)
        stream._ascii_buffer = list(updated_lines())


class stream(object):
    def __init__(self, args):
        self.args = args
        self.csv_options = dict(
            full_xpath=False,
            binary=self.args.binary,
            flush=self.args.flush,
            delimiter=self.args.delimiter,
            precision=self.args.precision,
            verbose=self.args.verbose)
        self.initialize_input()
        self.initialize_update_and_output()
        if self.args.verbose:
            self.print_info()

    def initialize_input(self):
        self.nonblank_input_fields = filter(None, self.args.fields)
        if not self.nonblank_input_fields:
            raise csv_eval_error("specify input stream fields, e.g. --fields=x,y")
        check_fields(self.nonblank_input_fields)
        types = comma.csv.format.to_numpy(self.args.format)
        self.input_t = comma.csv.struct(','.join(self.args.fields), *types)
        self.input = comma.csv.stream(self.input_t, **self.csv_options)

    def initialize_update_and_output(self):
        if self.args.select or self.args.exit_if:
            return
        if self.args.update_fields:
            all_types = comma.csv.format.to_numpy(self.args.format)
            index = self.args.fields.index
            update_types = [all_types[index(f)] for f in self.args.update_fields]
            update_fields = ','.join(self.args.update_fields)
            self.update_t = comma.csv.struct(update_fields, *update_types)
        if self.args.output_fields:
            check_output_fields(self.args.output_fields, self.nonblank_input_fields)
            output_types = comma.csv.format.to_numpy(self.args.output_format)
            output_fields = ','.join(self.args.output_fields)
            self.output_t = comma.csv.struct(output_fields, *output_types)
            self.output = comma.csv.stream(self.output_t,
                                           tied=self.input,
                                           **self.csv_options)

    def print_info(self, file=sys.stderr):
        fields = ','.join(self.input_t.nondefault_fields)
        format = self.input_t.format
        print >> file, "expressions: '{}'".format(self.args.expressions)
        print >> file, "select: '{}'".format(self.args.select)
        print >> file, "exit_if: '{}'".format(self.args.exit_if)
        print >> file, "default values: '{}'".format(self.args.default_values)
        print >> file, "input fields: '{}'".format(fields)
        print >> file, "input format: '{}'".format(format)
        if self.args.select or self.args.exit_if:
            return
        update_fields = ','.join(self.update_t.fields) if self.args.update_fields else ''
        output_fields = ','.join(self.output_t.fields) if self.args.output_fields else ''
        output_format = self.output_t.format if self.args.output_fields else ''
        print >> file, "update fields: '{}'".format(update_fields)
        print >> file, "output fields: '{}'".format(output_fields)
        print >> file, "output format: '{}'".format(output_format)


def check_fields(fields, allow_numpy_names=True):
    for field in fields:
        if not re.match(r'^[a-z_]\w*$', field, re.I):
            raise csv_eval_error("'{}' is not a valid field name".format(field))
        if field in ['_input', '_update', '_output']:
            raise csv_eval_error("'{}' is a reserved name".format(field))
        if not allow_numpy_names and field in np.__dict__:
            raise csv_eval_error("'{}' is a reserved numpy name".format(field))


def check_output_fields(fields, input_fields):
    check_fields(fields)
    invalid_output_fields = set(fields).intersection(input_fields)
    if invalid_output_fields:
        msg = "output fields '{}' are present in input fields '{}'" \
            .format(','.join(invalid_output_fields), ','.join(input_fields))
        raise csv_eval_error(msg)


def evaluate(stream):
    def disperse(var, fields):
        return '\n'.join("{f} = {v}['{f}']".format(v=var, f=f) for f in fields)
    def collect(var, fields):
        return '\n'.join("{v}['{f}'] = {f}".format(v=var, f=f) for f in fields)
    code_string = '\n'.join([stream.args.default_values,
                             disperse('_input', stream.nonblank_input_fields),
                             stream.args.expressions,
                             collect('_update', stream.args.update_fields),
                             collect('_output', stream.args.output_fields)])
    code = compile(code_string, '<string>', 'exec')
    env = np.__dict__ if stream.args.permissive else restricted_numpy_env()
    size = None
    update = None
    output = None
    is_shutdown = comma.signal.is_shutdown()
    while not is_shutdown:
        input = stream.input.read()
        if input is None:
            break
        if size != input.size:
            size = input.size
            if stream.args.update_fields:
                update = stream.update_t(size)
            if stream.args.output_fields:
                output = stream.output_t(size)
        exec(code in env, {'_input': input, '_update': update, '_output': output})
        if stream.args.update_fields:
            update_buffer(stream.input, update)
        if stream.args.output_fields:
            stream.output.write(output)
        else:
            stream.input.dump()


def select(stream):
    env = restricted_numpy_env()
    exec(stream.args.default_values in env)
    fields = stream.input.fields
    code = compile(stream.args.select, '<string>', 'eval')
    is_shutdown = comma.signal.is_shutdown()
    while not is_shutdown:
        input = stream.input.read()
        if input is None:
            break
        mask = eval(code, env, {f: input[f] for f in fields})
        stream.input.dump(mask=mask)

def exit_if(stream):
    env = restricted_numpy_env()
    exec stream.args.default_values in env
    fields = stream.input.fields
    code = compile(stream.args.exit_if, '<string>', 'eval')
    is_shutdown = comma.signal.is_shutdown()
    while not is_shutdown:
        input = stream.input.read(size=1)
        if input is None:
            break
        mask = eval(code, env, {f: input[f] for f in fields})
        if mask:
            if not stream.args.with_error: sys.exit()
            name = os.path.basename(sys.argv[0])
            print >> sys.stderr, "{} error: {}".format(name, stream.args.with_error)
            sys.exit(1)
        stream.input.dump()

def main():
    try:
        comma.csv.time.zone('UTC')
        args = get_args()
        prepare_options(args)
        if args.select:
            select(stream(args))
        elif args.exit_if:
            exit_if(stream(args))
        else:
            evaluate(stream(args))
    except csv_eval_error as e:
        name = os.path.basename(sys.argv[0])
        print >> sys.stderr, "{} error: {}".format(name, e)
        sys.exit(1)
    except StandardError as e:
        import traceback
        traceback.print_exc(file=sys.stderr)
        sys.exit(1)

if __name__ == '__main__':
    main()
