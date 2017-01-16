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

import numpy as np
from ..numpy import types_of_dtype, structured_dtype, type_to_string


class struct(object):
    """
    see github.com/acfr/comma/wiki/python-csv-module for details
    """
    default_field_name = 'comma_struct_default_field_name_'

    def __init__(self, concise_fields, *concise_types):
        self.concise_types = concise_types
        self.concise_fields = self._fill_blanks(concise_fields)
        self._check_fields_conciseness()
        #self.dtype = np.dtype(zip(self.concise_fields, self.concise_types))
        self.dtype = np.dtype([(x,y) for x,y in zip(self.concise_fields, self.concise_types)])
        self.fields = self._full_xpath_fields()
        self.nondefault_fields = self._nondefault_fields()
        self.types = self._basic_types()
        self.shorthand = self._shorthand()
        self.format = ','.join(self.types)
        #self.flat_dtype = np.dtype(zip(self.fields, self.types))
        self.flat_dtype = np.dtype([(x,y) for x,y in zip(self.fields, self.types)])
        unrolled_types = types_of_dtype(self.flat_dtype, unroll=True)
        self.unrolled_flat_dtype = structured_dtype(','.join(unrolled_types))
        self.type_of_field = dict(zip(self.fields, self.types))
        leaves = tuple(xpath.split('/')[-1] for xpath in self.fields)
        self.ambiguous_leaves = set(leaf for leaf in leaves if leaves.count(leaf) > 1)
        self.xpath_of_leaf = self._xpath_of_leaf(leaves)

    def __call__(self, size=1):
        return np.empty(size, dtype=self)

    def to_tuple(self, s):
        """
        convert a scalar or 1d array of dtype defined by struct to tuple

        >>> import comma
        >>> struct = comma.csv.struct('a,b', 'S2', 'u4')
        >>> data = struct()
        >>> data['a'] = 'ab'
        >>> data['b'] = 12
        >>> struct.to_tuple(data)
        ('ab', 12L)
        """
        if s.dtype != self.dtype:
            msg = "expected {}, got {}".format(repr(self.dtype), repr(s.dtype))
            raise TypeError(msg)
        if not (s.shape == (1,) or s.shape == ()):
            msg = "expected a scalar or 1d array with size=1, got shape={}".format(s.shape)
            raise ValueError(msg)
        return s.view(self.unrolled_flat_dtype).item()

    def expand_shorthand(self, compressed_fields):
        """
        return tuple of full-xpath fields corresponding to the given shorthand

        >>> import comma
        >>> inner = comma.csv.struct('i,j', 'u1', 'u1')
        >>> outer = comma.csv.struct('in', inner)
        >>> outer.expand_shorthand('in')
        ('in/i', 'in/j')
        """
        if isinstance(compressed_fields, str):
            compressed_fields = compressed_fields.split(',')
        expand = self.shorthand.get
        field_tuples = map(lambda name: expand(name) or (name,), compressed_fields)
        return sum(field_tuples, ())

    def _nondefault_fields(self):
        default_name = struct.default_field_name
        return tuple(map(lambda f: '' if f.startswith(default_name) else f, self.fields))

    def _fill_blanks(self, fields):
        if isinstance(fields, str):
            fields = fields.split(',')
        ntypes = len(self.concise_types)
        if len(fields) > ntypes:
            fields_without_type = ','.join(fields[ntypes:])
            msg = "missing types for fields '{}'".format(fields_without_type)
            raise ValueError(msg)
        omitted_fields = [''] * (ntypes - len(fields))
        fields_without_blanks = []
        for index, field in enumerate(fields + omitted_fields):
            if field:
                nonblank_field = field
            else:
                nonblank_field = '{}{}'.format(struct.default_field_name, index)
            fields_without_blanks.append(nonblank_field)
        return fields_without_blanks

    def _check_fields_conciseness(self):
        for field in self.concise_fields:
            if '/' in field:
                msg = "expected fields without '/', got '{}'".format(field)
                raise ValueError(msg)

    def _full_xpath_fields(self):
        fields = []
        for name, type in zip(self.concise_fields, self.concise_types):
            if isinstance(type, struct):
                fields_of_type = [name + '/' + field for field in type.fields]
                fields.extend(fields_of_type)
            else:
                fields.append(name)
        return tuple(fields)

    def _basic_types(self):
        types = []
        for type in self.concise_types:
            if isinstance(type, struct):
                types.extend(type.types)
            else:
                types.append(type_to_string(type))
        return tuple(types)

    def _shorthand(self):
        shorthand = {}
        for name, type in zip(self.concise_fields, self.concise_types):
            if not isinstance(type, struct):
                continue
            fields_of_type = [name + '/' + field for field in type.fields]
            shorthand[name] = tuple(fields_of_type)
            for subname, subfields in type.shorthand.iteritems():
                xpath = name + '/' + subname
                shorthand[xpath] = tuple(name + '/' + field for field in subfields)
        return shorthand

    def _xpath_of_leaf(self, leaves):
        xpath_of_leaf = dict(zip(leaves, self.fields))
        for ambiguous_leaf in self.ambiguous_leaves:
            del xpath_of_leaf[ambiguous_leaf]
        return xpath_of_leaf
