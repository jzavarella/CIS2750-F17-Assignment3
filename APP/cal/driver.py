#!/usr/bin/python3
from ctypes import *

class CalendarDriver(object):
    """docstring for CalendarDriver."""
    def __init__(self, sofile):
        self.sofile = sofile
        self.calLib = CDLL(sofile)
        # Set return and argument types
        self.calLib.printError.restype = c_char_p
        self.calLib.validateCalendarPython.restype = c_uint;
        self.calLib.validateCalendarPython.argtypes = [c_char_p]
        self.calLib.getCalendarComponentsPython.restype = c_char_p
        self.calLib.getCalendarComponentsPython.argtypes = [c_char_p]

    def validateCalendar(self, fileName):
        response = self.calLib.validateCalendarPython(self.encode(fileName))
        return self.calLib.printError(response).decode()

    def getCalendarComponents(self, fileName):
        response = self.calLib.getCalendarComponentsPython(self.encode(fileName))
        components = response.decode().split("\"\\")
        parsedComponents = []
        for component in components:
            if component == '':
                continue
            parsedComponents.append(component.split("\\\"")) # Add this component to the list
        return parsedComponents


    def printError(self, error):
        return self.calLib.printError(response).decode()

    def encode(self, string):
        return string.encode('utf-8')




# calDriver = CalendarDriver("bin/libcparse.so")
# print(calDriver.validateCalendar("cals/valid_one_alarm.ics"))

# # cdll.LoadLibrary("libllist.a")
# calLib = CDLL("bin/libcparse.so")
# #
# # calLib.test.restype = c_wchar_p
# # ret = calLib.test(c_wchar_p("hello"))
#
#
#
#
# calLib.printError.restype = c_char_p
# ret = calLib.printError(INV_FILE)
#
#
# calLib.createCalendarPython.restype = c_uint;
# calLib.createCalendarPython.argtypes = [c_char_p]
# string = "cals/valid_one_alarm.ics"
# bstring = string.encode('utf-8')
# response = calLib.createCalendarPython(bstring)
# print(response)
# retString = calLib.printError(response)
# print(retString.decode())
