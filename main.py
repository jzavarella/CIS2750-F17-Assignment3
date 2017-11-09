#!/usr/bin/python3
from ctypes import *
# cdll.LoadLibrary("libllist.a")
listlib = CDLL("listlib.so")
temp = listlib.initializeNode(50)
print(temp)
