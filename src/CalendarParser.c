/*
 * CIS2750 F2017
 * Assignment 2
 * Jackson Zavarella 0929350
 * This file parses iCalendar Files
 * No code was used from previous classes/ sources
 */

#define _GNU_SOURCE

#include <regex.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "CalendarParser.h"
#include "HelperFunctions.h"

ICalErrorCode validateCalendarPython(char* fileName) {
  Calendar* c = calloc(sizeof(Calendar), 1);
  ICalErrorCode e = createCalendar(fileName, &c);
  deleteCalendar(c);
  return e;
}

char* getCalendarComponentsPython(char* fileName) {
  Calendar* c = calloc(sizeof(Calendar), 1);
  ICalErrorCode e = createCalendar(fileName, &c);
  if (e != OK) {
    return NULL;
  }

  ListIterator events = createIterator(c->events);
  Event* event;
  int components = 0;
  size_t strlength = 0;
  while ((event = nextElement(&events))) { // Loop over all events
    components ++; // Increment number of components
    int propNumber = getLength(event->properties) + 2; // +2 for the two required props
    int alarmNumber = getLength(event->alarms);
    strlength += snprintf(NULL, 0, "%d", propNumber);
    strlength += strlen("\\\""); // Deliminate the elements with a string of illegal characters fro ical file
    strlength += snprintf(NULL, 0, "%d", alarmNumber);
    Property* summary;
    strlength += strlen("\\\"");
    if ((summary = findElement(event->properties, &compareTags, "SUMMARY"))) {
      strlength += strlen(summary->propDescr);
    }
    strlength += strlen("\"\\");
  }
  strlength ++; // Make room for null terminator

  char* final = calloc(strlength + 1, 1);
  strcpy(final, "");
  events = createIterator(c->events);
  while ((event = nextElement(&events))) { // Loop over all events
    components ++; // Increment number of components
    int propNumber = getLength(event->properties) + 2; // +2 for the two required props
    int alarmNumber = getLength(event->alarms);
    char temp[100];
    sprintf(temp, "%d", propNumber);
    strcat(final, temp);
    strcat(final, "\\\"");
    sprintf(temp, "%d", alarmNumber);
    strcat(final, temp);
    Property* summary;
    strcat(final, "\\\"");
    if ((summary = findElement(event->properties, &compareTags, "SUMMARY"))) {
      strcat(final, summary->propDescr);
    }
    strcat(final, "\"\\");
  }

  deleteCalendar(c);
  return final;
}

/** Function to create a Calendar object based on the contents of an iCalendar file.
 *@pre File name cannot be an empty string or NULL.  File name must have the .ics extension.
       File represented by this name must exist and must be readable.
 *@post Either:
        A valid calendar has been created, its address was stored in the variable obj, and OK was returned
		or
		An error occurred, the calendar was not created, all temporary memory was freed, obj was set to NULL, and the
		appropriate error code was returned
 *@return the error code indicating success or the error encountered when parsing the calendar
 *@param fileName - a string containing the name of the iCalendar file
 *@param a double pointer to a Calendar struct that needs to be allocated
**/
ICalErrorCode createCalendar(char* fileName, Calendar** obj) {
  *obj = calloc(sizeof(Calendar), 1);
  Calendar* calendar = *obj;

  strcpy(calendar->prodID, ""); // Ensure that this field is not blank to prevent uninitialized conditional jump errors in valgrind
  calendar->version = -1;
  List events = initializeList(&printEventListFunction, &deleteEventListFunction, &compareEventListFunction);
  calendar->events = events; // Assign the empty event list

  // List to store calendar properties
  List betweenVCalendarTags = initializeList(&printPropertyListFunction, &deletePropertyListFunction, &comparePropertyListFunction);
  calendar->properties = betweenVCalendarTags;
  List iCalPropertyList = initializeList(&printPropertyListFunction, &deletePropertyListFunction, &comparePropertyListFunction); // Create a list to store all properties/ lines
  ICalErrorCode lineCheckError = readLinesIntoList(fileName, &iCalPropertyList, 512); // Read the lines of the file into a list of properties
  if (lineCheckError != OK) { // If any of the lines were invalid, this will not return OK
    clearList(&iCalPropertyList); // Clear the list before returning
    return lineCheckError; // Return the error that was produced
  }


  // Get the properties between event tags
  ICalErrorCode betweenVCalendarTagsError = extractBetweenTags(iCalPropertyList, &betweenVCalendarTags, INV_CAL, "VCALENDAR");
  if (betweenVCalendarTagsError != OK) { // If there was a problem parsing
    clearList(&iCalPropertyList); // Free lists before returning
    clearList(&betweenVCalendarTags);
    return betweenVCalendarTagsError; // Return the error that was produced
  }

  // List to store event properties
  List betweenVEventTags = initializeList(&printPropertyListFunction, &deletePropertyListFunction, &comparePropertyListFunction);

  ICalErrorCode eventTagsError;
  // While we still have VALARM tags
  while ((eventTagsError = extractBetweenTags(betweenVCalendarTags, &betweenVEventTags, INV_EVENT, "VEVENT")) != INV_EVENT) {
    // If there is an event, check the event error
    if (eventTagsError != OK) {
      clearList(&iCalPropertyList); // Clear lists before returning
      clearList(&events);
      clearList(&betweenVCalendarTags);
      clearList(&betweenVEventTags);
      return eventTagsError; // Return the error that was produced
    }
    Event* event = newEmptyEvent();

    ICalErrorCode eventError = createEvent(betweenVEventTags, event); // Create and event given the event properties that were extracted
    if (eventError != OK) { // If there was a problem
      clearList(&iCalPropertyList); // Clear list before returning
      clearList(&events);
      clearList(&betweenVCalendarTags);
      clearList(&betweenVEventTags);
      events.deleteData(event); // Remove the event
      return eventError; // Return the error that was produced
    }

    insertBack(&events, event); // Put her in

    removeIntersectionOfLists(&betweenVCalendarTags, betweenVEventTags);
    deleteProperty(&betweenVCalendarTags, "BEGIN:VEVENT"); // Remove the begin tag
    deleteProperty(&betweenVCalendarTags, "END:VEVENT"); // Remove the begin tag
    clearList(&betweenVEventTags);

  }

  // Check to see if there is an event at all
  if (!events.head) {
    clearList(&iCalPropertyList); // Clear lists before returning
    clearList(&betweenVCalendarTags);
    clearList(&betweenVEventTags);
    clearList(&events);
    return INV_CAL; // If there is no event, then the calendar is invalid
  }

  // // We should only have VERSION and PRODID now
  ICalErrorCode iCalIdErrors = parseRequirediCalTags(&betweenVCalendarTags, *obj); // Place UID and version in the obj
  if (iCalIdErrors != OK) { // If there was a problem
    clearList(&iCalPropertyList); // Clear lists before returning
    clearList(&betweenVCalendarTags);
    clearList(&betweenVEventTags);
    clearList(&events);
    return iCalIdErrors; // Return the error that was produced
  }

  clearList(&iCalPropertyList); // Clear lists before returning
  clearList(&betweenVEventTags);
  calendar->properties = betweenVCalendarTags; // Set the remaining properties to the calendar
  calendar->events = events; // Assign the empty event list

  return validateCalendar(calendar);
}

/** Function to delete all calendar content and free all the memory.
 *@pre Calendar object exists, is not null, and has not been freed
 *@post Calendar object had been freed
 *@return none
 *@param obj - a pointer to a Calendar struct
**/
void deleteCalendar(Calendar* obj) {
  if (obj == NULL) {
    return; // No need to be freed if the object is NULL
  }
  List events = obj->events;
  clearList(&events);
  List properties = obj->properties;
  clearList(&properties);

  free(obj); // Free object? I Like free objects
}

/** Function to create a string representation of a Calendar object.
 *@pre Calendar object exists, is not null, and is valid
 *@post Calendar has not been modified in any way, and a string representing the Calndar contents has been created
 *@return a string contaning a humanly readable representation of a Calendar object
 *@param obj - a pointer to a Calendar struct
**/
char* printCalendar(const Calendar* obj) {
  if (!obj) {
    return NULL; // If the object does not exist dont do anything
  }

  if (validateCalendar(obj) != OK) { // Validate the calendar before outputting
    return NULL;
  }

  char* string;
  size_t stringSize = 0; // Total size of the completed string
  size_t lineLength = 0; // Size of the current line we are calculating
  size_t longestLine = 0; // Length of the lonest line

  // PRODUCT ID: Something\n
  if (strlen(obj->prodID) == 0) {
    return NULL; // Must have a prodID
  }
  calculateLineLength(&lineLength, " PRODID:", obj->prodID, "\n", NULL); // Add the length of these strings to the lineLength
  updateLongestLineAndIncrementStringSize(&longestLine, &lineLength, &stringSize); // Update the variable that stores the line with the greatest length

  // VERSION: 2.0\n
  if (!obj->version) {
    return NULL; // Must have a version
  }
  // Make room for the version string
  char vString[snprintf(NULL, 0, "%f", obj->version) + 1];
  snprintf(vString, sizeof(vString) + 1, "%f", obj->version);

  calculateLineLength(&lineLength, " VERSION:", vString, "\n", NULL); // Add the length of these strings to the lineLength
  updateLongestLineAndIncrementStringSize(&longestLine, &lineLength, &stringSize); // Update the variable that stores the line with the greatest length

  List events = obj->events;
  ListIterator eventIter = createIterator(events);
  Event* event;

  while ((event = nextElement(&eventIter))) {
    calculateLineLength(&lineLength, " CALENDAR EVENT:\n" , NULL); // Add the length of these strings to the lineLength
    updateLongestLineAndIncrementStringSize(&longestLine, &lineLength, &stringSize); // Update the variable that stores the line with the greatest length

    // UID: some uid\n
    if (strlen(event->UID) == 0) {
      return NULL;
    }

    calculateLineLength(&lineLength, "  UID:", event->UID, "\n" , NULL); // Add the length of these strings to the lineLength
    updateLongestLineAndIncrementStringSize(&longestLine, &lineLength, &stringSize); // Update the variable that stores the line with the greatest length

    // CREATION TIMESTAMP: some time\n
    char* dtString = printDatePretty(event->creationDateTime);
    calculateLineLength(&lineLength, "  DTSTAMP:", dtString, "\n" , NULL); // Add the length of these strings to the lineLength
    safelyFreeString(dtString);
    updateLongestLineAndIncrementStringSize(&longestLine, &lineLength, &stringSize); // Update the variable that stores the line with the greatest length

    List alarms = event->alarms;
    if (alarms.head) {
      ListIterator alarmIterator = createIterator(alarms);

      Alarm* a;
      while ((a = nextElement(&alarmIterator)) != NULL) { // Loop through each alarm
        calculateLineLength(&lineLength, "  ALARM:\n" , NULL); // Add the length of these strings to the lineLength
        updateLongestLineAndIncrementStringSize(&longestLine, &lineLength, &stringSize); // Update the variable that stores the line with the greatest length

        if (strlen(a->action) == 0) {
          return NULL; // Action is empty return null
        }
        // Get the length of the Action line
        calculateLineLength(&lineLength, "   ACTION:", a->action, "\n" , NULL); // Add the length of these strings to the lineLength
        updateLongestLineAndIncrementStringSize(&longestLine, &lineLength, &stringSize); // Update the variable that stores the line with the greatest length

        if (strlen(a->trigger) == 0) {
          return NULL; // Action is empty return null
        }
        // Get the length of the Trigger line
        calculateLineLength(&lineLength, "   TRIGGER:", a->trigger ,"\n" , NULL); // Add the length of these strings to the lineLength
        updateLongestLineAndIncrementStringSize(&longestLine, &lineLength, &stringSize); // Update the variable that stores the line with the greatest length

        List alarmProps = a->properties;
        // Output the props
        if (alarmProps.head) {
          calculateLineLength(&lineLength, "   ALARM PROPERTIES:\n" , NULL); // Add the length of these strings to the lineLength
          updateLongestLineAndIncrementStringSize(&longestLine, &lineLength, &stringSize); // Update the variable that stores the line with the greatest length

          // Get length of each property
          ListIterator propsIter = createIterator(alarmProps);
          Property* p;

          while ((p = nextElement(&propsIter)) != NULL) {
            char* printedProp = printPropertyListFunction(p); // Get the string for this prop
            calculateLineLength(&lineLength, "      ", printedProp, "\n", NULL); // Add the length of these strings to the lineLength
            updateLongestLineAndIncrementStringSize(&longestLine, &lineLength, &stringSize); // Update the variable that stores the line with the greatest length
            safelyFreeString(printedProp); // Free the string
          }
        }
      }
    }

    List propsList = event->properties;
    if (propsList.head) {

      calculateLineLength(&lineLength, "  EVENT PROPERTIES:\n", NULL); // Add the length of these strings to the lineLength
      updateLongestLineAndIncrementStringSize(&longestLine, &lineLength, &stringSize); // Update the variable that stores the line with the greatest length

      // Get length of each property
      ListIterator propsIter = createIterator(propsList);
      Property* p;
      while ((p = nextElement(&propsIter)) != NULL) {
        char* printedProp = printPropertyListFunction(p); // Get the string for this prop
        calculateLineLength(&lineLength, "   ", printedProp, "\n", NULL); // Add the length of these strings to the lineLength
        updateLongestLineAndIncrementStringSize(&longestLine, &lineLength, &stringSize); // Update the variable that stores the line with the greatest length
        safelyFreeString(printedProp); //  Free the string
      }
    }
  }

  List calPropsList = obj->properties;
  if (calPropsList.head) {

    calculateLineLength(&lineLength, " CALENDAR PROPERTIES:\n", NULL); // Add the length of these strings to the lineLength
    updateLongestLineAndIncrementStringSize(&longestLine, &lineLength, &stringSize); // Update the variable that stores the line with the greatest length

    // Get length of each property
    ListIterator propsIter = createIterator(calPropsList);
    Property* p;
    while ((p = nextElement(&propsIter)) != NULL) {
      char* printedProp = printPropertyListFunction(p); // Get the string for this prop
      calculateLineLength(&lineLength, "  ", printedProp, "\n", NULL); // Add the length of these strings to the lineLength
      updateLongestLineAndIncrementStringSize(&longestLine, &lineLength, &stringSize); // Update the variable that stores the line with the greatest length
      safelyFreeString(printedProp); //  Free the string
    }
  }

  longestLine += 1; // Make room for null terminator

  //Create the cap and footer of the string representation using by concatonating '-' n times for however long the longest line is
  char cap[longestLine];
  for (size_t i = 0; i < longestLine - 1; i++) {
    cap[i] = '-';
  }
  cap[longestLine - 1] = '\n';
  cap[longestLine] = '\0';

  string = malloc(stringSize * sizeof(char) + (2 * longestLine) + 1); // Allocate memory for the completed string

  strcpy(string, cap); // Header

  // PRODUCT ID: Something\n
  concatenateLine(string, " PRODID:", obj->prodID, "\n", NULL);
  // VERSION: 2.0\n
  concatenateLine(string, " VERSION:", vString, "\n", NULL);

  eventIter = createIterator(events);

  while ((event = nextElement(&eventIter))) {
    concatenateLine(string, " CALENDAR EVENT:\n", NULL);
    // UID: some uid\n
    concatenateLine(string, "  UID:", event->UID, "\n", NULL);
    // CREATION TIMESTAMP: some time\n
    char* dtString = printDatePretty(event->creationDateTime);
    concatenateLine(string, "  DTSTAMP:", dtString, "\n", NULL);
    safelyFreeString(dtString);

    List alarms = event->alarms;
    if (alarms.head) {
      ListIterator alarmIterator = createIterator(alarms);

      Alarm* a;
      while ((a = nextElement(&alarmIterator)) != NULL) { // Loop through each alarm
        concatenateLine(string, "  ALARM:\n", NULL); // Alarm header
        concatenateLine(string, "   ACTION:", a->action, "\n", NULL); // Alarm action
        concatenateLine(string, "   TRIGGER:", a->trigger, "\n", NULL); // Alarm trigger

        List alarmProps = a->properties;
        // Output the props
        if (alarmProps.head) {
          concatenateLine(string, "   ALARM PROPERTIES:\n", NULL); // Alarm properties header

          // Get each property string
          ListIterator propsIter = createIterator(alarmProps);
          Property* p;

          while ((p = nextElement(&propsIter)) != NULL) {
            char* printedProp = printPropertyListFunction(p);
            concatenateLine(string, "    ", printedProp, "\n", NULL); // Alarm properties
            safelyFreeString(printedProp); // Free the string
          }
        }
      }
    }

    // EVENT PROPERTIES: \n
    List propsList = event->properties;
    if (propsList.head) {
      concatenateLine(string, "  EVENT PROPERTIES:\n", NULL); // Event properties header

      // print each property
      ListIterator propsIter = createIterator(propsList);
      Property* p;

      while ((p = nextElement(&propsIter)) != NULL) {
        char* printedProp = printPropertyListFunction(p);
        concatenateLine(string, "   ", printedProp, "\n", NULL); // Event properties
        safelyFreeString(printedProp);
      }
    }
  }

  // EVENT PROPERTIES: \n
  if (calPropsList.head) {
    concatenateLine(string, " CALENDAR PROPERTIES:\n", NULL); // Event properties header

    // print each property
    ListIterator propsIter = createIterator(calPropsList);
    Property* p;

    while ((p = nextElement(&propsIter)) != NULL) {
      char* printedProp = printPropertyListFunction(p);
      concatenateLine(string, "  ", printedProp, "\n", NULL); // Event properties
      safelyFreeString(printedProp);
    }
  }

  strcat(string, cap); // Footer

  return string; // Beam me up
}


/** Function to "convert" the ICalErrorCode into a humanly redabale string.
 *@return a string contaning a humanly readable representation of the error code by indexing into
          the descr array using rhe error code enum value as an index
 *@param err - an error code
**/
char* printError(ICalErrorCode err) {
  char* error;
  switch (err) {
    case OK: // OK
      error = malloc(sizeof("OK"));
      strcpy(error, "OK");
      break;
    case INV_FILE: // INV_FILE
      error = malloc(sizeof("Invalid File"));
      strcpy(error, "Invalid File");
      break;
    case INV_CAL: // INV_CAL
      error = malloc(sizeof("Invalid Calendar"));
      strcpy(error, "Invalid Calendar");
      break;
    case INV_VER: // INV_VER
      error = malloc(sizeof("Malformed Version"));
      strcpy(error, "Malformed Version");
      break;
    case DUP_VER: // DUP_VER
      error = malloc(sizeof("Duplicate Version"));
      strcpy(error, "Duplicate Version");
      break;
    case INV_PRODID: // INV_PRODID
      error = malloc(sizeof("Malformed Product ID"));
      strcpy(error, "Malformed Product ID");
      break;
    case DUP_PRODID: // DUP_PRODID
      error = malloc(sizeof("Duplicate Product ID"));
      strcpy(error, "Duplicate Product ID");
      break;
    case INV_EVENT: // INV_EVENT
      error = malloc(sizeof("Invalid Event"));
      strcpy(error, "Invalid Event");
      break;
    case INV_CREATEDT: // INV_CREATEDT
      error = malloc(sizeof("Malformed Date"));
      strcpy(error, "Malformed Date");
      break;
    case INV_ALARM:
      error = malloc(sizeof("Alarm Error"));
      strcpy(error, "Alarm Error");
      break;
    case WRITE_ERROR:
      error = malloc(sizeof("File Write Error"));
      strcpy(error, "File Write Error");
      break;
    case OTHER_ERROR:
      error = malloc(sizeof("Generic Error")); // This error may be generic, but you aren't :)
      strcpy(error, "Generic Error");
      break;
    default:
      error = malloc(sizeof("NULL"));
      strcpy(error, "NULL");
      break;
  }
  return error;
}

/** Function to writing a Calendar object into a file in iCalendar format.
 *@pre Calendar object exists, is not null, and is valid
 *@post Calendar has not been modified in any way, and a file representing the
        Calendar contents in iCalendar format has been created
 *@return the error code indicating success or the error encountered when parsing the calendar
 *@param obj - a pointer to a Calendar struct
 **/
ICalErrorCode writeCalendar(char* fileName, const Calendar* obj) {

  if (!match(fileName, ".+\\.ics$")) { // If the file name does not match the valid ical regex
    return INV_FILE;
  }

  FILE* file;
  if (!((file = fopen(fileName, "w+")))) { // If the file cannot be opened
    return WRITE_ERROR;
  }

  ICalErrorCode error = validateCalendar(obj); // Validate the calendar
  if (error != OK) {
    fclose(file); // close the file before returning
    return error;
  }

  char* string = printCalendar(obj);
  if (!string) { // If we couldnt parse the calendar into a string
    fclose(file); // close the file before returning
    return OTHER_ERROR;
  }

  // This structure acts like a sudo stack to determine which tag we should have open
  char stack[3][80];
  strcpy(stack[0], "VCALENDAR");
  strcpy(stack[1], "VEVENT");
  strcpy(stack[2], "VALARM");

  int stackPointer = 0; // This integer will point to the top of the stack

  const char deliminer[2] = "\n"; // Break the lines of the to string at newline

  fprintf(file, "%s%s\r\n", "BEGIN:", stack[stackPointer]); // Write the begin tag which will always be BEGIN:VCALENDAR in this case
  // push stack
  stackPointer ++;

  char *line;
  line = strtok(string, deliminer);
  int previousSpaces = 0; // Will hold the spaces preceeding the string on the previous line
  int currentSpaces = 0; // Will hold the current spaces at the beginning of the line
  while(line != NULL) { // Process each line of the output
    if (!match(line, "^-+$")) { // Dont process the caps of the string
      currentSpaces = getSpaces(line); // Get the currentSpaces at the beginning of the line
      /*
        BELOW IS THE MAIN ALGORITHM
        It essentially checks to see how many spaces at the beginning of a line and depending
        on if the current spaces are greater than or less than the previous spaces it knows
        whether to add a begin tag or close the previous tag. The algorithm also skips cosmetic lines in
        the toString that should not be in the icalendar file. (ex: 'ALARM/EVENT PROPERTIES:' etc...)
      */
      if (previousSpaces <= currentSpaces) {
        if (match(line, "CALENDAR EVENT:") || (match(line, "ALARM:"))) {
          // push the stack
          fprintf(file, "%s%s\r\n", "BEGIN:", stack[stackPointer]);
          stackPointer ++;
        } else if (match(line, "ALARM PROPERTIES:") || match(line, "EVENT PROPERTIES:") || match(line, "CALENDAR PROPERTIES:")) {
          previousSpaces = currentSpaces;
          line = strtok(NULL, deliminer);
          continue; // we dont want to print these lines
        } else {
          size_t len = strlen(line);
          memmove(line, line+currentSpaces, len - currentSpaces + 1); // Remove spaces from the beginning of line
          fprintf(file, "%s\r\n", line);
        }
      } else if (previousSpaces > currentSpaces) {
        int difference = previousSpaces - currentSpaces;
        do {
          // pop the stack
          stackPointer --;
          fprintf(file, "%s%s\r\n", "END:", stack[stackPointer]);
          difference --;
        } while(difference > 1);
        if (match(line, "CALENDAR EVENT:") || (match(line, "ALARM:"))) {
          // push the stack
          fprintf(file, "%s%s\r\n", "BEGIN:", stack[stackPointer]);
          stackPointer ++;
        } else if (match(line, "ALARM PROPERTIES:") || match(line, "EVENT PROPERTIES:") || match(line, "CALENDAR PROPERTIES:")) {
          previousSpaces = currentSpaces;
          line = strtok(NULL, deliminer); // We dont want to print these lines
          continue;
        }
      }
    }
    previousSpaces = currentSpaces;
    line = strtok(NULL, deliminer);
  }
  while (stackPointer != 0) { // While we still have items in the stack
    stackPointer --;
    fprintf(file, "%s%s\r\n", "END:", stack[stackPointer]);
  }
  free(string); // Free string
  fclose(file); // Close file before returning
  return OK;
}

int getPropCount(char* propName, List props) {
  ListIterator propIter = createIterator(props);
  Property* prop;
  int count = 0;
  while ((prop = nextElement(&propIter))) {
    if (strcasecmp(propName, prop->propName) == 0) {
      count ++;
    }
  }
  return count;
}

ICalErrorCode validateEventProps(const Calendar* obj, Event* event) {
  ListIterator eventPropIter = createIterator(event->properties);
  Property* prop;

  while ((prop = nextElement(&eventPropIter))) {
    char* propName = prop->propName;
    char* propDescr = prop->propDescr;
    if (match(propName, "^ATTACH$") && matchURIField(propDescr)) {
      continue;
    } else if (match(propName, "^CATEGORIES$") && matchTEXTListField(propDescr)) {
      continue;
    } else if (match(propName, "^CLASS$") && match(propDescr, "^(PUBLIC|PRIVATE|CONFIDENTIAL)$")) {
      continue;
    } else if (match(propName, "^COMMENT$") && matchTEXTField(propDescr)) {
      continue;
    } else if (match(propName, "^DESCRIPTION$") && matchSUMMARYField(propDescr) && getPropCount("DESCRIPTION", event->properties) <= 1) {
      continue;
    } else if (match(propName, "^GEO$") && matchLONGLATField(propDescr)) {
      continue;
    } else if (match(propName, "^LOCATION$") && matchTEXTField(propDescr)) {
      continue;
    } else if (match(propName, "^PRIORITY$") && match(propDescr, "^[[:digit:]]$")) {
      continue;
    } else if (match(propName, "^RESOURCES$") && matchTEXTListField(propDescr) && getPropCount("RESOURCES", event->properties) <= 1) {
      continue;
    } else if (match(propName, "^STATUS$") && match(propDescr, "^(TENTATIVE|CONFIRMED|CANCELLED)$") && getPropCount("STATUS", event->properties) <= 1) {
      continue;
    } else if (match(propName, "^SUMMARY$") && matchSUMMARYField(propDescr)) {
      continue;
    } else if (match(propName, "^DTEND$") && matchDATEField(propDescr) && getPropCount("DTSTART", event->properties) == 1) { // DTEND requires that we have a start
      continue;
    } else if (match(propName, "^DTSTART$") && matchDATEField(propDescr) && getPropCount("DTSTART", event->properties) <= 1) {
      continue;
    } else if (match(propName, "^DURATION$") && matchDURATIONField(propDescr)) {
      continue;
    } else if (match(propName, "^TRANSP$") && match(propDescr, "^(OPAQUE|TRANSPARENT)$") && getPropCount("TRANSP", event->properties) <= 1) {
      continue;
    } else if (match(propName, "^ATENDEE$") && matchTEXTField(propDescr)) {
      continue;
    } else if (match(propName, "^CONTACT$") && matchTEXTField(propDescr)) {
      continue;
    } else if (match(propName, "^ORGANIZER$") && matchEMAILField(propDescr)) {
      continue;
    } else if (match(propName, "^RELATED-TO$") && matchTEXTField(propDescr)) {
      continue;
    } else if (match(propName, "^URL$") && matchURIField(propDescr) && getPropCount("URL", event->properties) <= 1) {
      continue;
    } else if (match(propName, "^EXDATE$") && matchTEXTField(propDescr)) {
      continue;
    } else if (match(propName, "^RDATE$") && matchTEXTField(propDescr)) {
      continue;
    } else if (match(propName, "^RRULE") && matchTEXTField(propDescr) && getPropCount("RRULE", event->properties) <= 1) {
      continue;
    } else if (match(propName, "CREATED") && matchDATEField(propDescr) && getPropCount("CREATED", event->properties) <= 1) {
     continue;
    } else if (match(propName, "LAST-MODIFIED") && matchDATEField(propDescr)) {
     continue;
   } else if (match(propName, "SEQUENCE") && match(propDescr, "^[[:digit:]]$")) {
     continue;
    } else {
      // printf("INV EVENT: %s %s\n", propName, propDescr);
      return INV_EVENT;
    }
  }

  return OK;
}

ICalErrorCode validateAlarmProps(const Calendar* obj, Event* event, Alarm* alarm) {
  ListIterator alarmPropIter = createIterator(alarm->properties);
  Property* prop;

  while ((prop = nextElement(&alarmPropIter))) {
    char* propName = prop->propName;
    char* propDescr = prop->propDescr;
    if (match(propName, "^ATTACH$") && matchURIField(propDescr)) {
      continue;
    } else if (match(propName, "^REPEAT$") && match(propDescr, "^[[:digit:]]+$")) {
      continue;
    } else if (match(propName, "^DURATION$") && matchDURATIONField(propDescr)) {
      continue;
    } else if (match(propName, "^DESCRIPTION$") && matchSUMMARYField(propDescr) && getPropCount("DESCRIPTION", alarm->properties) <= 1) {
      continue;
    } else if (match(propName, "^SUMMARY$") && matchTEXTField(propDescr)) {
      continue;
    } else {
      // printf("INV ALARM: %s %s\n", propName, propDescr);
      return INV_ALARM;
    }
  }

  return OK;
}

ICalErrorCode validateCalProps(const Calendar* obj) {
  ListIterator calPropIter = createIterator(obj->properties);
  Property* prop;

  while ((prop = nextElement(&calPropIter))) {
    char* propName = prop->propName;
    char* propDescr = prop->propDescr;
    if (match(propName, "^CALSCALE$") && matchTEXTField(propDescr) && getPropCount("CALSCALE", obj->properties) <= 1) {
      continue;
    } else if (match(propName, "^METHOD$") && matchTEXTField(propDescr) && getPropCount("METHOD", obj->properties) <= 1) {
      continue;
    } else {
      // printf("INV CAL: %s %s\n", propName, propDescr);
      return INV_CAL;
    }
  }
  return OK;
}

/** Function to validating an existing a Calendar object
 *@pre Calendar object exists and is not null
 *@post Calendar has not been modified in any way
 *@return the error code indicating success or the error encountered when validating the calendar
 *@param obj - a pointer to a Calendar struct
 **/
ICalErrorCode validateCalendar(const Calendar* obj) {
  if (!obj) {
    return OTHER_ERROR; // If the sent object is null
  }

  if (!obj->version) {
    return INV_VER; // Must have a version
  }

  if (strcmp(obj->prodID, "") == 0) {
    return INV_CAL; // prodID if missing
  }

  if (!matchTEXTField(obj->prodID)) {
    return INV_PRODID; // prodID is malformed
  }

  if (getLength(obj->events) < 1) {
    return INV_CAL; // Must have at least one event
  }

  ListIterator eventIter = createIterator(obj->events);
  Event* ev;

  while ((ev = nextElement(&eventIter))) { // Loop through all events
    if (strlen(ev->UID) < 1) {
      return INV_EVENT; // UID cannot be blank
    }
    DateTime dt = ev->creationDateTime;
    if (!match(dt.date, "^[[:digit:]]{8}$") || !match(dt.time, "^[[:digit:]]{6}$")) { // Match valid dates
      return INV_CREATEDT;
    }

    ICalErrorCode eventPropsError = validateEventProps(obj, ev);
    if (eventPropsError != OK) {
      return eventPropsError;
    }

    ListIterator alarmIter = createIterator(ev->alarms);
    Alarm* a;
    while ((a = nextElement(&alarmIter))) {
      // if (!match(a->action, "^(AUDIO|DISPLAY|EMAIL)$") || (strcmp(a->trigger, "") == 0)) {
      //   return INV_ALARM;
      // }
      //
      // ListIterator aPropIter = createIterator(a->properties);
      // Property* ap;
      //
      // while ((ap = nextElement(&aPropIter))) {
      //   if (strlen(ap->propName) == 0) {
      //     return INV_ALARM;
      //   }
      // }
      ICalErrorCode alarmErrorCode = validateAlarmProps(obj, ev, a);
      if (alarmErrorCode != OK) {
        return alarmErrorCode;
      }
    }
  }

  ICalErrorCode calPropsError = validateCalProps(obj);
  if (calPropsError != OK) {
    return calPropsError;
  }

  return OK;

}

// <------START OF HELPER FUNCTIONS----->

/** Function to match the given string to the regex expression
  Returns 1 if the string matches the pattern is a match
  Returns 0 if the string does not match the pattern
*/
int match(const char* string, char* pattern) {
  int status;
  regex_t regex;
  int d;
  if ((d = regcomp(&regex, pattern, REG_EXTENDED|REG_NOSUB|REG_ICASE)) != 0) {
    return 0;
  }

  status = regexec(&regex, string, (size_t) 0, NULL, 0);
  regfree(&regex);
  if (status != 0) {
    return 0;
  }
  return(1);
}

int matchTEXTField(const char* line) {
  return match(line, "^(;|:){0,1}[^[:cntrl:]\"\\,:;]+$"); // This regex matches valid text lines
}

int matchSUMMARYField(const char* line) {
  return match(line, "[^[:cntrl:]\"\\:;]+$"); // This regex matches valid text lines
}

int matchDATEField(const char* line) {
  return match(line, "(:|;){0,1}[[:digit:]]{8}T[[:digit:]]{6}Z{0,1}$"); // This regex matches valid date lines
}

int matchURIField(char* line) { //~~~~~~~~Scheme~~~//~AUTHORTY~~~~~~~~~~~~~~~~URL~~~~~~~~~~.extension~~~~~~~~~or~~IP Address~~~~~~~~~~~~~~~~~~~~~~~~~~Port~~~~~~~~~~~~~~~~PATH~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~File extension
  return match(line, "[[:alpha:]][[:alnum:]+-:\\.]*//([[:alnum:]]+:.+@){0,1}([[:alnum:]]+(\\.[[:alpha:]]+){0,1}|[[:digit:]]{1,3}(\\.[[:digit:]]+){3,})(:[[:digit:]]+){0,1}(/([[:alnum:]-]+/{0,1})+){0,1}(\\.[[:alnum:]]+)*$");
}

int matchTEXTListField(char* line) {
  return match(line, "^[^[:cntrl:]\"\\,:;]+(,[^[:cntrl:]\"\\,:;]+)*$");
}

int matchLONGLATField(char* line) {
  return match(line, "^[\\+\\-]{0,1}[[:digit:]]+(\\.[[:digit:]]+){0,1};[\\+\\-]{0,1}[[:digit:]]+(\\.[[:digit:]]+){0,1}$");
}

int matchEMAILField(char* line) {
  return match(line, "[[:alnum:]\\.\\-]+@[[:alnum:]\\-]+\\.[[:alnum:]]+(\\.[[:alnum:]]+){0,1}$");
}

int matchDURATIONField(char* line) {
  // Second: [[:digit:]]+S
  // minute: [[:digit:]]+M([[:digit:]]+S){0,1}
  // hour: [[:digit:]]+H([[:digit:]]+M([[:digit:]]+S){0,1}){0,1}
  // time: (T[[:digit:]]+H([[:digit:]]+M([[:digit:]]+S){0,1}){0,1}|T[[:digit:]]+M([[:digit:]]+S){0,1}|T[[:digit:]]+S)
  // day: [[:digit:]]+D
  // date: [[:digit:]]+D((T[[:digit:]]+H([[:digit:]]+M([[:digit:]]+S){0,1}){0,1}|T[[:digit:]]+M([[:digit:]]+S){0,1}|T[[:digit:]]+S)){0,1}
  return match(line, "([\\+-]{0,1}P([[:digit:]]+D((T[[:digit:]]+H([[:digit:]]+M([[:digit:]]+S){0,1}){0,1}|T[[:digit:]]+M([[:digit:]]+S){0,1}|T[[:digit:]]+S)){0,1}|(T[[:digit:]]+H([[:digit:]]+M([[:digit:]]+S){0,1}){0,1}|T[[:digit:]]+M([[:digit:]]+S){0,1}|T[[:digit:]]+S)|[[:digit:]]+W)|(:|;){0,1}[[:digit:]]{8}T[[:digit:]]{6}Z{0,1})$");
}

// Check if the string is allocated before freeing it
void safelyFreeString(char* c) {
  if (c) {
    free(c);
  }
}

void deleteEventListFunction(void* toBeDeleted) {
  Event* event = (Event*) toBeDeleted;
  if (event != NULL) { // If there is an event
    List* props = &event->properties; // Grab the props
    if (props) { // If there are props
      clearList(props); // Set them free
    }
    List* alarms = &event->alarms; // Grab the alarms
    if (alarms) { // If the alarms exist
      clearList(alarms); // Set them free
    }
    free(event); // Set it free
  }
}

char* printEventListFunction(void *toBePrinted) {
  size_t longestLine = 0;
  size_t lineLength = 0;
  size_t stringSize = 0;
  Event* event = (Event*) toBePrinted;

  calculateLineLength(&lineLength, " CALENDAR EVENT: \n" , NULL); // Add the length of these strings to the lineLength
  updateLongestLineAndIncrementStringSize(&longestLine, &lineLength, &stringSize); // Update the variable that stores the line with the greatest length

  // UID: some uid\n
  if (strlen(event->UID) == 0) {
    return NULL;
  }

  calculateLineLength(&lineLength, "  UID: ", event->UID, "\n" , NULL); // Add the length of these strings to the lineLength
  updateLongestLineAndIncrementStringSize(&longestLine, &lineLength, &stringSize); // Update the variable that stores the line with the greatest length

  // CREATION TIMESTAMP: some time\n
  char* dtString = printDatePretty(event->creationDateTime);
  calculateLineLength(&lineLength, "  CREATION TIMESTAMP: ", dtString, "\n" , NULL); // Add the length of these strings to the lineLength
  safelyFreeString(dtString);
  updateLongestLineAndIncrementStringSize(&longestLine, &lineLength, &stringSize); // Update the variable that stores the line with the greatest length

  List alarms = event->alarms;
  if (alarms.head) {
    ListIterator alarmIterator = createIterator(alarms);

    Alarm* a;
    while ((a = nextElement(&alarmIterator)) != NULL) { // Loop through each alarm
      calculateLineLength(&lineLength, "  ALARM: \n" , NULL); // Add the length of these strings to the lineLength
      updateLongestLineAndIncrementStringSize(&longestLine, &lineLength, &stringSize); // Update the variable that stores the line with the greatest length

      if (strlen(a->action) == 0) {
        return NULL; // Action is empty return null
      }
      // Get the length of the Action line
      calculateLineLength(&lineLength, "    ACTION: ", a->action, "\n" , NULL); // Add the length of these strings to the lineLength
      updateLongestLineAndIncrementStringSize(&longestLine, &lineLength, &stringSize); // Update the variable that stores the line with the greatest length

      if (strlen(a->trigger) == 0) {
        return NULL; // Action is empty return null
      }
      // Get the length of the Trigger line
      calculateLineLength(&lineLength, "    TRIGGER: ", a->trigger ,"\n" , NULL); // Add the length of these strings to the lineLength
      updateLongestLineAndIncrementStringSize(&longestLine, &lineLength, &stringSize); // Update the variable that stores the line with the greatest length

      List alarmProps = a->properties;
      // Output the props
      if (alarmProps.head) {
        calculateLineLength(&lineLength, "    ALARM PROPERTIES: \n" , NULL); // Add the length of these strings to the lineLength
        updateLongestLineAndIncrementStringSize(&longestLine, &lineLength, &stringSize); // Update the variable that stores the line with the greatest length

        // Get length of each property
        ListIterator propsIter = createIterator(alarmProps);
        Property* p;

        while ((p = nextElement(&propsIter)) != NULL) {
          char* printedProp = printPropertyListFunction(p); // Get the string for this prop
          calculateLineLength(&lineLength, "      ", printedProp, "\n", NULL); // Add the length of these strings to the lineLength
          updateLongestLineAndIncrementStringSize(&longestLine, &lineLength, &stringSize); // Update the variable that stores the line with the greatest length
          safelyFreeString(printedProp); // Free the string
        }
      }
    }
  }

  List propsList = event->properties;
  if (propsList.head) {

    calculateLineLength(&lineLength, "    EVENT PROPERTIES: \n", NULL); // Add the length of these strings to the lineLength
    updateLongestLineAndIncrementStringSize(&longestLine, &lineLength, &stringSize); // Update the variable that stores the line with the greatest length

    // Get length of each property
    ListIterator propsIter = createIterator(propsList);
    Property* p;
    while ((p = nextElement(&propsIter)) != NULL) {
      char* printedProp = printPropertyListFunction(p); // Get the string for this prop
      calculateLineLength(&lineLength, "    ", printedProp, "\n", NULL); // Add the length of these strings to the lineLength
      updateLongestLineAndIncrementStringSize(&longestLine, &lineLength, &stringSize); // Update the variable that stores the line with the greatest length
      safelyFreeString(printedProp); //  Free the string
    }
  }

  char* string = malloc(stringSize + 1);

  concatenateLine(string, " CALENDAR EVENT: \n", NULL);
  // UID: some uid\n
  concatenateLine(string, "   UID: ", event->UID, "\n", NULL);
  // CREATION TIMESTAMP: some time\n
  dtString = printDatePretty(event->creationDateTime);
  concatenateLine(string, "  CREATION TIMESTAMP: ", dtString, "\n", NULL);
  safelyFreeString(dtString);

  alarms = event->alarms;
  if (alarms.head) {
    ListIterator alarmIterator = createIterator(alarms);

    Alarm* a;
    while ((a = nextElement(&alarmIterator)) != NULL) { // Loop through each alarm
      concatenateLine(string, "  ALARM: \n", NULL); // Alarm header
      concatenateLine(string, "    ACTION: ", a->action, "\n", NULL); // Alarm action
      concatenateLine(string, "    TRIGGER: ", a->trigger, "\n", NULL); // Alarm trigger

      List alarmProps = a->properties;
      // Output the props
      if (alarmProps.head) {
        concatenateLine(string, "    ALARM PROPERTIES: \n", NULL); // Alarm properties header

        // Get each property string
        ListIterator propsIter = createIterator(alarmProps);
        Property* p;

        while ((p = nextElement(&propsIter)) != NULL) {
          char* printedProp = printPropertyListFunction(p);
          concatenateLine(string, "      ", printedProp, "\n", NULL); // Alarm properties
          safelyFreeString(printedProp); // Free the string
        }
      }
    }
  }

  // EVENT PROPERTIES: \n
  propsList = event->properties;
  if (propsList.head) {
    concatenateLine(string, "  EVENT PROPERTIES: \n", NULL); // Event properties header

    // print each property
    ListIterator propsIter = createIterator(propsList);
    Property* p;

    while ((p = nextElement(&propsIter)) != NULL) {
      char* printedProp = printPropertyListFunction(p);
      concatenateLine(string, "   ", printedProp, "\n", NULL); // Event properties
      safelyFreeString(printedProp);
    }
  }
  return string;
}

char* printPropertyListFunction(void *toBePrinted) {
  Property* p = (Property*) toBePrinted;
  size_t finalSize = 0;
  char c;
  size_t i = 0;
  while ((c = p->propName[i]) != '\0') {
    i ++; // Calculate length of the prop name
  }
  finalSize += i; // add it to the total
  i = 0; // reset
  while ((c = p->propDescr[i]) != '\0') {
    i ++; // Calculate length of description
  }
  finalSize += i; // add it up again
  if (p->propDescr[0] != ';' && p->propDescr[0] != ':') {
    finalSize ++;
  }
  char* string = calloc(finalSize + 1, 1); // +1 to make room for NULL terminator
  strcpy(string, p->propName);
  if (p->propDescr[0] != ';' && p->propDescr[0] != ':') {
    strcat(string, ":");
  }
  strcat(string, p->propDescr);
  string[finalSize] = '\0'; // Set null terminator just in case strcat didnt
  return string;
}

int compareEventListFunction(const void *first, const void *second) {
  Event* e1 = (Event*) first;
  Event* e2 = (Event*) second;
  char* s1 = e1->UID;
  char* s2 = e2->UID;
  int result = strcmp(s1, s2); // Compare the strings

  safelyFreeString(s1); // BECAUSE I'M FREEEEE!!!!! FREE FALLIN'
  safelyFreeString(s2);
	return result; // Return the result
}

int comparePropertyListFunction(const void *first, const void *second) {
  char* s1 = printPropertyListFunction((void*)first); // Print each prop
  char* s2 = printPropertyListFunction((void*)second);
  int result = strcmp(s1, s2); // Compare the strings

  safelyFreeString(s1); // BECAUSE I'M FREEEEE!!!!! FREE FALLIN'
  safelyFreeString(s2);
	return result; // Return the result
}

void deletePropertyListFunction(void *toBeDeleted) {
  Property* p = (Property*) toBeDeleted;
  if (!p) {
    return; // Cant free nothing. Someone must have done our job before we got here >.>
  }
	free(p);
}

// This method never gets called but whatev
char* printAlarmListFunction(void *toBePrinted) {
  Alarm* a = (Alarm*) toBePrinted;
  size_t finalSize = 0;
  char c;
  size_t i = 0;
  while ((c = a->action[i]) != '\0') {
    i ++; // Calculate size of action
  }
  finalSize += i;
  i = 0;
  while ((c = a->trigger[i]) != '\0') {
    i ++; // Calculate size of trigger
  }
  finalSize += i;
  finalSize += 4; // Add room for "|"
  char* propListString = toString(a->properties); // Print the properties
  i = 0;
  while ((c = propListString[i]) != '\0') {
    i ++; // Calculate size of list string
  }
  finalSize += i;
  // Mash it all up together
  char* string = malloc(finalSize + 1); // +1 to make room for NULL terminator
  strcpy(string, "|");
  strcat(string, a->action);
  strcat(string, "|");
  strcat(string, a->trigger);
  strcat(string, "|");
  strcat(string, propListString);
  strcat(string, "|");
  string[finalSize] = '\0'; // Set null terminator just in case strcat didnt
  safelyFreeString(propListString); // Bye felicia
  return string; // Return the string
}

// This hasnt been required yet
int compareAlarmListFunction(const void *first, const void *second) {
  char* s1 = printAlarmListFunction((void*)first); // Print each prop
  char* s2 = printAlarmListFunction((void*)second);
  int result = strcmp(s1, s2); // Compare the strings

  safelyFreeString(s1); // BECAUSE I'M FREEEEE!!!!! FREE FALLIN'
  safelyFreeString(s2);
	return result; // Return the result
}

void deleteAlarmListFunction(void *toBeDeleted) {
  Alarm* a = (Alarm*) toBeDeleted;
  if (!a) {
    return; // Cant free nothing
  }
  if (a->trigger) {
    free(a->trigger); // Free trigger if it exists
  }
  clearList(&a->properties); // Clear properties
	free(a); // Bye
}

Property* createProperty(char* propName, char* propDescr) {
  Property* p = calloc(sizeof(Property) + strlen(propDescr)*sizeof(char*) + 1 /* this 1 is to make room for null terminator */ + strlen(propName), 1); // Allocate room for the property and the flexible array member
  strcpy(p->propName, propName); // Copy prop name over
  p->propDescr[0] = '\0';
  strcpy(p->propDescr, propDescr); // Copy prop description over
  return p; // Send it back
}

Alarm* createAlarm(char* action, char* trigger, List properties) {
  if (!action || !trigger) {
    return NULL; // If the action or trigger is null then nothing can save you
  }
  Alarm* alarm = calloc(sizeof(Alarm), 1);
  if (action[0] == ':' || action[0] == ';') { // Remove the beginning ; or : if it exists
    memmove(action, action + 1, strlen(action));
  }
  strcpy(alarm->action, action); // Copy the action
  alarm->trigger = calloc(strlen(trigger) + 1, 1); // Allocate room for trigger

  if (!alarm->trigger) {
    free(alarm); // Free alarm before returning
    return NULL; // If we were unable to allocate memory
  }

  if (trigger[0] == ':' || trigger[0] == ';') { // Remove the beginning ; or : if it exists
    memmove(trigger, trigger + 1, strlen(trigger));
  }
  strcpy(alarm->trigger, trigger); // Copy trigger
  alarm->properties = properties; // Set the properties

  return alarm; // Send it back
}

Alarm* createAlarmFromPropList(List props) {
  // Create a list for the props
  List alarmProps = initializeList(&printPropertyListFunction, &deletePropertyListFunction, &comparePropertyListFunction);

  char* ACTION = NULL; // Declare action and trigger
  char* TRIGGER = NULL;

  Property* prop; // Declare prop
  ListIterator propsIterator = createIterator(props); // Do I really need to re-iterate myself here?

  while ((prop = nextElement(&propsIterator)) != NULL) {
    char* propName = prop->propName; // Get name
    char* propDescr = prop->propDescr; // Get description

    if (strcmp(propDescr, "") == 0 || strcmp(propName, "") == 0) { // If no descripting, we are in trouble
      clearList(&alarmProps); // Clear before returning
      return NULL; // Bye
    }
    char tempDescription[strlen(propDescr) + 2]; //
    strcpy(tempDescription, propDescr);
    if (match(tempDescription, "^(;|:)")) {
      memmove(tempDescription, tempDescription+1, strlen(tempDescription)); // remove the first character as it is (; or :)
    }
    if (match(propName, "^ACTION$")) {
      if (ACTION || !match(tempDescription, "^(AUDIO|DISPLAY|EMAIL)$")) {
        clearList(&alarmProps);
        return NULL; // Already have an ACTION or description is null
      }
      ACTION = malloc(strlen(tempDescription) + 1);
      strcpy(ACTION, tempDescription);

    } else if (match(propName, "^TRIGGER$")) {
      if (TRIGGER || !matchDURATIONField(propDescr)) {
        clearList(&alarmProps);
        // printf("%s\n", propDescr);
        return NULL; // Already have trigger
      } else {
        TRIGGER = malloc(strlen(tempDescription) + 1);
        strcpy(TRIGGER, tempDescription);
      }
    } else {
      Property* p = createProperty(propName, propDescr);
      insertBack(&alarmProps, p);
    }
  }

  if (!ACTION || !TRIGGER) {
    clearList(&alarmProps);
    safelyFreeString(ACTION);
    safelyFreeString(TRIGGER);
    return NULL;
  }
  Alarm* a = createAlarm(ACTION, TRIGGER, alarmProps);
  safelyFreeString(ACTION);
  safelyFreeString(TRIGGER);
  return a;
}

char* extractSubstringBefore(char* line, char* terminator) {

  if (!line || !terminator) { // If the line is NULL return null
    return NULL;
  }

  int terminatorIndex = strcspn(line, terminator);
  size_t lineLength = strlen(line);

  if (terminatorIndex == lineLength) { // If there is no colon or the length is 0
    return NULL;
  }

  char* substring = malloc(terminatorIndex * sizeof(char) + 1); // Allocate that much room and add 1 for null terminator
  strncpy(substring, &line[0], terminatorIndex); // Copy 'size' chars into description starting from the index of ":" + 1
  substring[terminatorIndex] = '\0'; // Add null terminator because strncpy wont

  return substring;
}


char* extractSubstringAfter(char* line, char* terminator) {
  if (!line || !terminator) { // If the line is NULL return null
    return NULL;
  }

  int terminatorIndex = strcspn(line, terminator);
  size_t lineLength = strlen(line);

  if (terminatorIndex == lineLength) { // If there is no terminator or the length is 0
    return NULL;
  }

  size_t size = lineLength - (terminatorIndex + 1); // Calculate the number of chars in the final (+1 on terminator as to not include the colon)
  char* substring = malloc(size * sizeof(char) + 1); // Allocate that much room and add 1 for null terminator
  strncpy(substring, &line[terminatorIndex + 1], size); // Copy 'size' chars into description starting from the index of ":" + 1
  substring[size] = '\0'; // Add null terminator because strncpy wont
  return substring;
}

Property* extractPropertyFromLine(char* line) {
  size_t lineLength = strlen(line);
  char tempLine[lineLength + 1];
  strcpy(tempLine, line);
  char* propName = strtok(line, ":;");
  char* temp;
  size_t descriptionLength = 0;
  while ((temp = strtok(NULL, ";:"))) {
    descriptionLength += strlen(temp);
    descriptionLength += 1; // strtok removes the ';'
  }
  // descriptionLength += 1; // strtok removes the ';'
  char propDescr[descriptionLength + 1];
  size_t substring = lineLength - descriptionLength;
  if (match(&tempLine[substring], "^(;|:)")) {
    memcpy(propDescr, tempLine + substring + 1, descriptionLength);
  } else {
    memcpy(propDescr, tempLine + substring, descriptionLength);
  }
  propDescr[descriptionLength] = '\0';
  Property* p = createProperty(propName, propDescr);
  return p;
}

/**
  *Takes a file that has been opened and reads the lines into a linked list of chars*
  *with each node being one line of the file.
  *@param: file
  * The file to be read. Note the file must be open
  *@param: bufferSize
  * The maximum size of each line
  *@return: list
  * The list with each line read into it
*/
ICalErrorCode readLinesIntoList(char* fileName, List* list, int bufferSize) {
  FILE* file; // Going to be used to store the file

  // If the fileName is NULL or does not match the regex expression *.ics or cannot be opened
  if (!fileName || !match(fileName, ".+\\.ics$") || (file = fopen(fileName, "r")) == NULL) {

    return INV_FILE; // The file is invalid
  }

  char * line = NULL;
  size_t len = 0;
  ssize_t read;
  char prevLine[bufferSize * 10];
  strcpy(prevLine, "");
  FILE* tempFile = fopen("temp.ics", "w+");

  while ((read = getline(&line, &len, file)) != -1) {
    if (match(line, "^;.*\n\r$")) { // Line ending
      continue;
    }
    if (match(line, "^[[:blank:]]+.+")) { // if this line starts with a space or tab and isnt blank
      if (prevLine[strlen(prevLine) - 1] == '\n') { // Remove new line from end of line
        if ((int)strlen(prevLine) - 2 >= 0 && prevLine[strlen(prevLine) - 2] == '\r') { // Remove carriage return if it exists
          prevLine[strlen(prevLine) - 2] = '\0';
        }
      }
      memmove(line, line + 1, strlen(line)); // Shift the contents of the string right by one as to not include the space
      strcat(prevLine, line);
      continue;
    }

    // write line
    fputs(prevLine, tempFile);
    strcpy(prevLine, line);
  }
  fputs(prevLine, tempFile);
  safelyFreeString(line);
  fclose(tempFile);

  file = fopen("temp.ics", "r");

  line = NULL;
  len = 0;

  while ((read = getline(&line, &len, file)) != -1) {
    if (match(line, "^;")) {
      continue; // This is a line comment
    }
    if (match(line, "^[[:blank:]]+.+")) { // if this line starts with a space or tab and isnt blank
      Node* tailNode = list->tail;
      if (!tailNode) {
        fclose(file);
        return INV_CAL; // This is the first line in the list and the first line cannot be a line continuation
      }

      Property* p = (Property*) tailNode->data;
      if (!p) {
        fclose(file);
        return INV_CAL; // Something has gone wrong if the data is null
      }

      if (line[strlen(line) - 1] == '\n') { // Remove new line from end of line
        if ((int)strlen(line) - 2 >= 0 && line[strlen(line) - 2] == '\r') { // Remove carriage return if it exists
          line[strlen(line) - 2] = '\0';
        }
      }
      memmove(line, line + 1, strlen(line)); // Shift the contents of the string right by one as to not include the space
      size_t currentSize = sizeof(*p) + strlen(p->propDescr)*sizeof(char) + 1; // Calculatethe current size of the property
      size_t newSize = currentSize + strlen(line)*sizeof(char); // Calculate how much memory we need for the new property
      p = (Property*) tailNode->data;
      // tailNode->data = realloc(p, newSize + 1); // Reallocate memory for the new property
      p = realloc(p, newSize + 1); // Reallocate memory for new property
      strcat(p->propDescr, line); // Concat this line onto the property description

      tailNode->data = p;
      list->tail = tailNode;
      continue; // Continue to the next line
    }
    if (match(line, "(\r\n){0,1}$")) {
      if (line[strlen(line) - 1] == '\n') { // Remove new line from end of line
        if ((int)strlen(line) - 2 >= 0 && line[strlen(line) - 2] == '\r') { // Remove carriage return if it exists
          line[strlen(line) - 2] = '\0';
        } else {
          safelyFreeString(line);
          fclose(file);
          return INV_FILE;
        }
      }
    }
    if (match(line, "^[a-zA-Z\\-]*(:|;).*$")) {
      Property* p = extractPropertyFromLine(line);
      insertBack(list, p); // Insert the property into the list
    } else {
      safelyFreeString(line);
      fclose(file);
      return INV_CAL;
      // if (line[strlen(line) - 1] == '\n') { // Remove new line from end of line
      //   if ((int)strlen(line) - 2 >= 0 && line[strlen(line) - 2] == '\r') { // Remove carriage return if it exists
      //     line[strlen(line) - 2] = '\0';
      //   } else {
      //     safelyFreeString(line);
      //     fclose(file);
      //     return INV_FILE;
      //   }
      // }
      // Property* p = createProperty(line, "");
      // insertBack(list, p); // Insert the property into the list
    }
  }
  remove("temp.ics");
  safelyFreeString(line);
  fclose(file);
  if (!list->head) {
    return INV_CAL; // If the file was empty
  }
  return OK;
}

void deleteProperty(List* propList, char* line) {
  char temp[strlen(line) + 1]; // Make a temp variable that is allocated because extractPropertyFromLine uses strtok which cannot use a non existant memory address
  strcpy(temp, line);
  Property* p = extractPropertyFromLine(temp);
  safelyFreeString(deleteDataFromList(propList, p));
  free(p);
}

ICalErrorCode extractBetweenTags(List props, List* extracted, ICalErrorCode onFailError, char* tag) {
  clearList(extracted); // Clear the list just in case
  ListIterator propsIterator = createIterator(props);

  // Build the regex expressions for begin and end tags
  size_t tagSize = strlen(tag);
  size_t beginTagSize = (strlen("^BEGIN:$") + tagSize) * sizeof(char);
  char beginTag[beginTagSize];
  strcpy(beginTag, "^BEGIN:");
  strcat(beginTag, tag);
  strcat(beginTag, "$");

  size_t endTagSize = (strlen("^END:$") + tagSize) * sizeof(char);
  char endTag[endTagSize];
  strcpy(endTag, "^END:");
  strcat(endTag, tag);
  strcat(endTag, "$");

  Property* prop;
  // These two ints count as flags to see if we have come across an event open/close
  int beginCount = 0;
  int endCount = 0;
  while ((prop = nextElement(&propsIterator)) != NULL) {
    char* line = props.printData(prop);
    if (match(line, beginTag)) {
      beginCount ++; // Set begin flag
      if (beginCount != 1) {
        safelyFreeString(line);
        return onFailError; // Opened another event without closing the previous
      }
    } else if (match(line, endTag)) {
      endCount ++; // Set end flag
      if (endCount != beginCount) {
        safelyFreeString(line);
        return onFailError; // Closed an event without opening one
      }
      safelyFreeString(line);
      break; // We have parsed out the event
    } else if (beginCount == 1 && endCount == 0) { // If begin is 'open', add this line to the list
      Property* p = extractPropertyFromLine(line);
      insertBack(extracted, p);
    }
    safelyFreeString(line); // Bye
  }
  if (beginCount == 0 && endCount == 0) { // If we didnt parse any tags at all
    return onFailError;
  }

  if (beginCount == 1 && endCount != beginCount) { // If there is no matching end tag
    return onFailError;
  }

  return OK; // If we made it here, we have extracted between the tags
}

// Make a string that is pretty (Just like you)
char* printDatePretty(DateTime dt) {
  size_t size = 0;
  size += strlen(dt.date);
  size += strlen("// ");
  size += strlen(dt.time);
  if (dt.UTC) {
    size += strlen("Z");
  }

  char* string = malloc(size + 1 * (sizeof(char)));
  strcpy(string, dt.date);
  strcat(string, "T");
  strcat(string, dt.time);
  if (dt.UTC) {
    strcat(string, "Z");
  }

  return string;
}

int fileExists(char* file) {
  FILE* filePointer;
  if ((filePointer = fopen(file, "r")) != NULL) {
    fclose(filePointer);
    return 1;
  }
  return 0;
}

// Creates a time and puts it into the sent event
ICalErrorCode createTime(Event* event, char* timeString) {
  if (!timeString || !event || !matchDATEField(timeString)) { // If its null or doesnt match the required regex
    return INV_CREATEDT;
  }
  char* numberDate;
  char* timeS;
  if (match(timeString, "^(:|;)")) {
    numberDate = extractSubstringBefore(&timeString[1], "T"); // Get substring before the T but start past the (semi)colon
  } else {
    numberDate = extractSubstringBefore(timeString, "T"); // Get the substring before the T
  }
  char* temp = extractSubstringAfter(timeString, "T"); // Extract after the T
  if (match(timeString, "Z$")) { // If UTC
    timeS = extractSubstringBefore(temp, "Z"); // Get the time between the T and Z
    event->creationDateTime.UTC = true; // UTC is true
    safelyFreeString(temp); // Free temp
  } else {
    timeS = temp; // Get the time after T
    event->creationDateTime.UTC = false;
  }
  if (!numberDate || !temp || !timeS) { // We dont have all of the things we need
    safelyFreeString(numberDate); // Free before returning
    safelyFreeString(timeS);
    return INV_CREATEDT; // We failed :'(
  }
  strcpy(event->creationDateTime.date, numberDate); // Copy the values
  strcpy(event->creationDateTime.time, timeS);

  safelyFreeString(numberDate); // Free before returning
  safelyFreeString(timeS);
  return OK; // You're OK but I have a girlfriend, sorry
}

// Remove the properties that are in l2 from l1
void removeIntersectionOfLists(List* l1, List l2) {
  ListIterator eventIterator = createIterator(l2);
  char* line;
  Property* prop;
  while ((prop = nextElement(&eventIterator)) != NULL) {
    line = l1->printData(prop);
    deleteProperty(l1, line); // Delete this line from the iCalendar line list and free the data
    safelyFreeString(line);
  }
}

Event* newEmptyEvent() {
  Event* e = calloc(sizeof(Event), 1); // MAKE ROOM FOR ME, GOSH!
  e->properties = initializeList(&printPropertyListFunction, &deletePropertyListFunction, &comparePropertyListFunction); // Set the lists
  e->alarms = initializeList(&printAlarmListFunction, &deleteAlarmListFunction, &compareAlarmListFunction);
  return e; // We done
}

List copyPropList(List toBeCopied) {
  List newList = initializeList(&printPropertyListFunction, &deletePropertyListFunction, &comparePropertyListFunction); // Make a new list

  ListIterator iter = createIterator(toBeCopied); // Make an iterator for ... well ... iterating
  Property* p;

  while ((p = nextElement(&iter)) != NULL) {
    char* c = toBeCopied.printData(p); // Print the data
    Property* p = extractPropertyFromLine(c); // Extract a property from the line
    insertBack(&newList, p); // ... And the bus driver said, to the back, to the back
    safelyFreeString(c); // Bye
  }
  return newList; // Return the new list
}

ICalErrorCode createEvent(List eventList, Event* event) {
  if (!event) {
    return INV_EVENT;
  }

  // Creating a property list that wil store all of the alarm props
  List alarmPropList = initializeList(&printPropertyListFunction, &deletePropertyListFunction, &comparePropertyListFunction);
  // Copy a new event list
  List newEventList = copyPropList(eventList);

  // While we still have VALARM tags
  while (extractBetweenTags(newEventList, &alarmPropList, INV_EVENT, "VALARM") != INV_EVENT) {
    Alarm* a = createAlarmFromPropList(alarmPropList);

    if (a) {
      insertBack(&event->alarms, a); // Put the alarm in
      removeIntersectionOfLists(&newEventList, alarmPropList); // Remove all elements from alarmPropList in newEventList
      deleteProperty(&newEventList, "BEGIN:VALARM"); // Remove Begin and end tags
      deleteProperty(&newEventList, "END:VALARM");
    } else {
      clearList(&newEventList); // Clear lists before returning
      clearList(&alarmPropList);
      return INV_ALARM; // Invalid event
    }
    clearList(&alarmPropList); // Clear the list
  }

  ListIterator eventIterator = createIterator(newEventList); // Iterate over remaining props
  Property* prop;

  char* UID = NULL; // Placeholders for uid and dstamp
  char* DTSTAMP = NULL;

  while ((prop = nextElement(&eventIterator)) != NULL) {
    char* propName = prop->propName; // make these for better readability
    char* propDescr = prop->propDescr;
    if (strcmp(propDescr, "") == 0 || strcmp(propName, "") == 0) { // If no descripting, we are in trouble
      safelyFreeString(UID); // Free strings before returning
      safelyFreeString(DTSTAMP);
      clearList(&newEventList); // Clear list before returning
      return INV_EVENT;
    }
    if (match(propName, "^UID$")) { // If this is the UID
      if (UID != NULL || !propDescr || !strlen(propDescr)) { // If there is a problem with it
        safelyFreeString(UID); // Free strings before returning
        safelyFreeString(DTSTAMP);
        clearList(&newEventList); // Clear list before returning
        return INV_EVENT; // UID has already been assigned or propDesc is null or empty
      }
      if (match(propDescr, "^(;|:)")) { // If the description starts with (semi)colon
        char temp[strlen(propDescr)];
        memcpy(temp, propDescr + 1*sizeof(char), strlen(propDescr)); // Shift the memory to get rid of the (semi)colon
        strcpy(event->UID, temp); // Copy it over
      } else {
        strcpy(event->UID, propDescr); // Copy it over
      }

      UID = eventList.printData(prop); // Set the UID
    } else if (match(propName, "^DTSTAMP$")) {
      if (DTSTAMP != NULL || !propDescr) { // If the date is problematic
        safelyFreeString(UID); // Free before returning
        safelyFreeString(DTSTAMP);
        clearList(&newEventList);
        return INV_EVENT; // DTSTAMP has already been assigned or propDesc is null or empty
      }
      DTSTAMP = eventList.printData(prop); // Set the DTSTAMP flag
      ICalErrorCode e = createTime(event, propDescr);
      if (e != OK) {
        safelyFreeString(UID); // Free stored UID
        safelyFreeString(DTSTAMP); // Free stored DTSTAMP
        clearList(&newEventList);
        return e;
      }
    }
  }

  if (!UID || !DTSTAMP) { // If wecould not find UID or DTSTAMP
    safelyFreeString(UID); // Free stored UID
    safelyFreeString(DTSTAMP); // Free stored DTSTAMP
    clearList(&newEventList);
    return INV_EVENT;
  }

  deleteProperty(&newEventList, UID); // Delete UID from event properties
  deleteProperty(&newEventList, DTSTAMP); // Delete DTSTAMP from event properties
  safelyFreeString(UID); // Free stored UID
  safelyFreeString(DTSTAMP); // Free stored DTSTAMP

  event->properties = newEventList; // Set properties
  clearList(&alarmPropList);

  return OK;
}

ICalErrorCode parseRequirediCalTags(List* list, Calendar* cal) {
  ListIterator iterator = createIterator(*list); // Iterate over props
  Property* p;
  char* VERSION = NULL;
  char* PRODID = NULL;
  while ((p = nextElement(&iterator)) != NULL) {
    char* name = p->propName;
    char* description = p->propDescr;

    if (match(name, "^VERSION$")) {
      if (VERSION) {
        safelyFreeString(PRODID); // PROD might be allocated so we must remove it before returning
        safelyFreeString(VERSION); // Version might be allocated so we must remove it before returning
        return DUP_VER;
      }
      if (!description || !match(description, "^(:|;){0,1}[[:digit:]]+(\\.[[:digit:]]+)*$")) {
        safelyFreeString(PRODID); // PROD might be allocated so we must remove it before returning
        safelyFreeString(VERSION); // Version might be allocated so we must remove it before returning
        return INV_VER;
      }

      VERSION = malloc(strlen(description) * sizeof(char) + 1);
      if (match(description, "^(;|:)")) {
        memmove(VERSION, description+1, strlen(description)); // remove the first character as it is (; or :)
      } else {
        strcpy(VERSION, description);
      }
    } else if (match(name, "^PRODID$")) {
      if (PRODID) {
        safelyFreeString(PRODID); // PROD might be allocated so we must remove it before returning
        safelyFreeString(VERSION); // Version might be allocated so we must remove it before returning
        return DUP_PRODID;
      }

      if (!description || !matchTEXTField(description)) {
        safelyFreeString(PRODID); // PROD might be allocated so we must remove it before returning
        safelyFreeString(VERSION); // Version might be allocated so we must remove it before returning
        return INV_PRODID;
      }
      PRODID = malloc(strlen(description) * sizeof(char) + 1);
        if (match(description, "^(;|:)")) {
          memmove(PRODID, description+1, strlen(description)); // remove the first character as it is (; or :)
        } else {
          strcpy(PRODID, description);
        }
    }
  }

  if (!VERSION || !PRODID) {
    safelyFreeString(PRODID); // PROD might be allocated so we must remove it before returning
    safelyFreeString(VERSION); // Version might be allocated so we must remove it before returning

    return INV_CAL; // We are missing required tags
  }

  Property* prodID = findElement(*list, &compareTags, "PRODID"); // Find the tags for prodID and version for eleting
  Property* version = findElement(*list, &compareTags, "VERSION");

  char* prodIDLine = printPropertyListFunction(prodID); // print their lines
  char* versionLine = printPropertyListFunction(version);

  deleteProperty(list, versionLine); // Delete them
  deleteProperty(list, prodIDLine);

  safelyFreeString(prodIDLine); // Free memory
  safelyFreeString(versionLine);

  cal->version = atof(VERSION);
  strcpy(cal->prodID, PRODID);
  safelyFreeString(PRODID); // Free strings before returning
  safelyFreeString(VERSION);
  return OK;
}

void updateLongestLineAndIncrementStringSize(size_t* longestLine, size_t* lineLength, size_t* stringSize) {
  if (*lineLength > *longestLine) { // If this line is greater than the previous longest line
    *longestLine = *lineLength; // Congrats, you are the new longest line
  }
  *stringSize += *lineLength; // Add this line length to the string size
  *lineLength = 0; // Reset the line length
}

void calculateLineLength(size_t* lineLength, const char* c, ... ) {
   va_list valist;
   va_start(valist, c);

   // Iterate over all variable params
   while (c) {
        *lineLength += strlen(c); // Add the length of this string to the total
        c = va_arg(valist, const char*); // Move to the next
    }
   // Clear the va_list
   va_end(valist);
}

void concatenateLine(char* string, const char* c, ... ) {
   va_list valist;
   va_start(valist, c);

    // Iterate over all variable params
   while (c) {
        strcat(string, c); // concatenate each value onto the string
        c = va_arg(valist, const char*); // Move to the next
    }
   // Clear the va_list
   va_end(valist);
}

bool compareTags(const void* first, const void* second) {
  Property* p = (Property*) first;
  char* propName = p->propName;
  if (strcmp(propName, (char*) second) == 0) {
    return true;
  }
  return false;
}

int getSpaces(char* line) {
  int count = 0;
  char c;
  while ((c = line[count]) == ' ') {
    count ++;
  }
  return count;
}

// If you made it this far, you win. Too bad the prize is nothing
