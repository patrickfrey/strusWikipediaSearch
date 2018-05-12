import collections
import numbers
import sys
import os

def _dumpTree( indent, obj, depth, excludeList):
	if obj is None:
		return "None"
	elif isinstance(obj, numbers.Number):
		if isinstance(obj, numbers.Integral):
			return "%d" % int(obj)
		elif isinstance(obj, numbers.Real) and obj.is_integer():
			return "%d" % int(obj)
		else:
			return "%.5f" % obj
	elif isinstance(obj, str):
		return '"' + obj + '"'
	elif isinstance(obj, bytes):
		return '"' + obj.decode('utf-8') + '"'
	elif isinstance(obj, collections.Sequence):
		if (depth == 0):
			return "{...}"
		s = ''
		i = 0
		for v in obj:
			ke = "\n" + indent + "number " + str(i+1)
			ve = _dumpTree( indent + '  ', v, depth-1, excludeList)
			if ve and ve[0] == "\n":
				s = s + ke + ":" + ve
			else:
				s = s + ke + ": " + ve
			i = i + 1
		return s
	elif isinstance(obj, dict):
		if (depth == 0):
			return "{...}"
		s = ''
		for k in sorted( obj.keys()):
			kstr = str(k)
			if kstr[0] == '_':
				continue
			if kstr in excludeList:
				continue
			ke = "\n" + indent + type(k).__name__ + " " + kstr
			ve = _dumpTree( indent + '  ', obj[ k], depth-1, excludeList)
			if ve and ve[0] == "\n":
				s = s + ke + ":" + ve
			else:
				s = s + ke + ": " + ve
		return s
	else:
		attrdict = {}
		for k in dir(obj):
			if k[0] != '_' and getattr( obj, k) != None:
				attrdict[ k] = getattr( obj, k)
		return _dumpTree( indent, attrdict, depth, excludeList)

def dumpTree( obj):
	return _dumpTree( "", obj, 20, {})
