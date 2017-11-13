#!/usr/bin/python3
from ctypes import *

class CalendarDriver(object):
    """docstring for CalendarDriver."""
    def __init__(self, sofile):
        self.sofile = sofile
        self.calLib = CDLL(sofile)

        self.calLib.openCalendarPython.restype = c_void_p
        self.calLib.openCalendarPython.argtypes = [c_char_p]
        self.calLib.deleteCalendar.argtypes = [c_void_p]
        self.calLib.writeCalendar.argtypes = [c_char_p, c_void_p]
        self.calLib.writeCalendar.restype = c_uint

        # Set return and argument types
        self.calLib.printError.restype = c_char_p
        self.calLib.validateCalendarPython.restype = c_uint;
        self.calLib.validateCalendarPython.argtypes = [c_char_p]
        self.calLib.getCalendarComponentsPython.restype = c_char_p
        self.calLib.getCalendarComponentsPython.argtypes = [c_void_p]
        self.calLib.getComponentPropsPython.restype = c_char_p
        self.calLib.getComponentPropsPython.argtypes = [c_void_p, c_int]
        self.calLib.getComponentAlarmsPython.restype = c_char_p
        self.calLib.getComponentAlarmsPython.argtypes = [c_void_p, c_int]
        self.calLib.validateVersionPython.restype = c_int
        self.calLib.validateVersionPython.argtypes = [c_char_p]
        self.calLib.matchTEXTField.restype = c_int
        self.calLib.matchTEXTField.argtypes = [c_char_p]
        self.calLib.matchDATEField.restype = c_int
        self.calLib.matchDATEField.argtypes = [c_char_p]

        self.calLib.createBasicCalendarPython.restype = c_void_p
        self.calLib.createBasicCalendarPython.argtypes = [c_float, c_char_p]

        self.calLib.addEventPython.argtypes = [c_void_p, c_char_p, c_char_p, c_char_p]
        self.calLib.safelyFreeString.argtypes = [c_void_p]

    def addEvent(self, calPointer, uid, date, start):
        self.calLib.addEventPython(calPointer, self.encode(uid), self.encode(date), self.encode(start))

    def createBasicCalendar(self, version, prodId):
        return self.calLib.createBasicCalendarPython(c_float(version), self.encode(prodId))

    def writeCalendar(self, fileName, calPointer):
        response = self.calLib.writeCalendar(self.encode(fileName), calPointer)
        return self.printError(response)

    def printError(self, error):
        charp = self.calLib.printError(error)
        humanReadable = charp.decode()
        # self.calLib.safelyFreeString(charp)
        return humanReadable

    def openCalendar(self, fileName):
        return self.calLib.openCalendarPython(self.encode(fileName))

    def validateCalendar(self, fileName):
        response = self.calLib.validateCalendarPython(self.encode(fileName))
        return self.printError(response)

    def getCalendarComponents(self, calPointer):
        response = self.calLib.getCalendarComponentsPython(calPointer)
        components = response.decode().split("\"\\")
        parsedComponents = []
        for component in components:
            if component == '':
                continue
            parsedComponents.append(component.split("\\\"")) # Add this component to the list
        return parsedComponents

    def deleteCalendar(self, calPointer):
        self.calLib.deleteCalendar(calPointer)

    def getComponentAlarms(self, calPointer, compNum):
        response = self.calLib.getComponentAlarmsPython(calPointer, c_int(compNum))
        if response is None:
            return None
        return "Component Alarms:\n" + response.decode()

    def getComponentProperties(self, calPointer, compNum):
        response = self.calLib.getComponentPropsPython(calPointer, c_int(compNum))
        if response is None:
            return None
        return "Component Properties:\n" + response.decode()

    def encode(self, string):
        return string.encode('utf-8')

    def validateVersion(self, version):
        response = self.calLib.validateVersionPython(self.encode(version))
        if response == 1:
            return True
        return False

    def matchDATEField(self, date):
        response = self.calLib.matchDATEField(self.encode(date))
        if response == 1:
            return True
        return False

    def matchTEXTField(self, text):
        response = self.calLib.matchTEXTField(self.encode(text))
        if response == 1:
            return True
        return False
