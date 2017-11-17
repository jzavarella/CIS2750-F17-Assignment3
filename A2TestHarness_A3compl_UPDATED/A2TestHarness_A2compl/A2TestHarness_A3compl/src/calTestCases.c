#include "LinkedListAPI.h"
#include "CalendarParser.h"
#include "testharness.h"
#include "../include/CalendarParser.h"
#include "../include/testharness.h"

#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>

#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/wait.h>

char * errVals[] = {
        "OK", "INV_FILE", "INV_CAL", "INV_VER", "DUP_VER", "INV_PRODID", "DUP_PRODID", "INV_EVENT", "INV_CREATEDT", "INV_ALARM", "WRITE_ERROR", "OTHER_ERROR"
};
char* tPrintError(ICalErrorCode err){
    return errVals[err];
}

Node* tInitializeNode(void* data){
    Node* tmpNode;

    tmpNode = (Node*)malloc(sizeof(Node));

    if (tmpNode == NULL){
        return NULL;
    }

    tmpNode->data = data;
    tmpNode->previous = NULL;
    tmpNode->next = NULL;

    return tmpNode;
}

List tInitializeList(char* (*printFunction)(void* toBePrinted),void (*deleteFunction)(void* toBeDeleted),int (*compareFunction)(const void* first,const void* second)){
    List tmpList;

    tmpList.length = 0;
    tmpList.head = NULL;
    tmpList.tail = NULL;
    tmpList.deleteData = deleteFunction;
    tmpList.compare = compareFunction;
    tmpList.printData = printFunction;

    return tmpList;
}

/**Inserts a Node at the front of a linked list.  List metadata is updated
 * so that head and tail pointers are correct.
 *@pre 'List' type must exist and be used in order to keep track of the linked list.
 *@param list pointer to the dummy head of the list
 *@param toBeAdded a pointer to data that is to be added to the linked list
 **/
void tInsertBack(List* list, void* toBeAdded){
    if (list == NULL || toBeAdded == NULL){
        return;
    }

    Node* newNode = tInitializeNode(toBeAdded);
    list->length += 1;
    if (list->head == NULL && list->tail == NULL){
        list->head = newNode;
        list->tail = list->head;
    }else{
        newNode->previous = list->tail;
        list->tail->next = newNode;
        list->tail = newNode;
    }

}

/**Inserts a Node at the front of a linked list.  List metadata is updated
 * so that head and tail pointers are correct.
 *@pre 'List' type must exist and be used in order to keep track of the linked list.
 *@param list pointer to the dummy head of the list
 *@param toBeAdded a pointer to data that is to be added to the linked list
 **/
void tInsertFront(List* list, void* toBeAdded){
    if (list == NULL || toBeAdded == NULL){
        return;
    }

    Node* newNode = tInitializeNode(toBeAdded);
    list->length += 1;
    if (list->head == NULL && list->tail == NULL){
        list->head = newNode;
        list->tail = list->head;
    }else{
        newNode->next = list->head;
        list->head->previous = newNode;
        list->head = newNode;
    }
}

bool propEqual(const Property* prop1, const Property* prop2){
    if (prop1 == NULL || prop2 == NULL){
        return false;
    }

    if ((strcmp(prop1->propName, prop2->propName) == 0) && (strcmp(prop1->propDescr, prop2->propDescr) == 0)){
        return true;
    }else{
        return false;
    }
}

bool containsProp(List list, const Property* prop){
    Node* ptr = list.head;

    while(ptr != NULL){
        Property* currProp = (Property*)ptr->data;
        if (propEqual(prop, currProp)){
            return true;
        }
        ptr = ptr->next;
    }
    return false;
}

Property* createTestProp(char* propName, char* propDescr){
    Property* prop;

    prop = malloc(sizeof(Property) + (sizeof(char)*(strlen(propDescr)+1)) );
    strcpy(prop->propName, propName);
    strcpy(prop->propDescr, propDescr);

    return prop;
}

bool dtEqual(DateTime testDT, DateTime refDT){
    //printf("Dates: %s %s\n", testDT.date, refDT.date);
    //printf("Times: %s %s\n", testDT.time, refDT.time);
    if (strcmp(testDT.date, refDT.date) == 0 &&
        strcmp(testDT.time, refDT.time) == 0 &&
        testDT.UTC == refDT.UTC){
        return true;
    }else{
        return false;
    }

}

bool pListEqual(List testList, List refList){

    Node* ptr = refList.head;
    while(ptr != NULL){
        //For every reference property, see if the test prop list contains it
        Property* currRefProp = (Property*)ptr->data;
        if (!containsProp(testList, currRefProp)){
           //printf("TEST LIST PROPS not equal %s:%s\n", currRefProp->propName, currRefProp->propDescr);
            return false;
        }
        ptr = ptr->next;
    }

    ptr = testList.head;
    while(ptr != NULL){
        //For every test property, see if the reference prop list contains it
        Property* currTestProp = (Property*)ptr->data;
        if (!containsProp(refList, currTestProp)){
            //printf("REF LIST PROPS not equal %s:%s\n", currTestProp->propName, currTestProp->propDescr);
            return false;
        }
        ptr = ptr->next;
    }

    return true;
}

bool alarmEqual(const Alarm* testAlarm, const Alarm* refAlarm){
    if (testAlarm == NULL || refAlarm == NULL){
        return false;
    }

    //Compare action
    if (strcmp(testAlarm->action, refAlarm->action) != 0){
        //printf("ALARM action not equal %s : %s\n", testAlarm->action, refAlarm->action);
        return false;
    }

    //Compare trigger
    if (testAlarm->trigger == NULL || refAlarm->trigger == NULL || strcmp(testAlarm->action, refAlarm->action) != 0){
        //printf("ALARM trigger not equal %s %s\n", testAlarm->trigger, refAlarm->trigger);
        return false;
    }

    //Compare properties
    if (!pListEqual(testAlarm->properties, refAlarm->properties)){
        //printf("ALARM: Properties not equal\n");
        return false;
    }

    return true;
}

bool containsAlarm(List list, const Alarm* alarm){
    Node* ptr = list.head;

    while(ptr != NULL){
        Alarm* currAlarm = (Alarm*)ptr->data;
        if (alarmEqual(alarm, currAlarm)){
            return true;
        }
        ptr = ptr->next;
    }
    return false;
}

bool aListEqual(List testList, List refList){

    Node* ptr = refList.head;
    while(ptr != NULL){
        //For every reference alarm, see if the test alarm list contains it
        Alarm* currRefAlarm = (Alarm*)ptr->data;
        if (!containsAlarm(testList, currRefAlarm)){
            return false;
        }
        ptr = ptr->next;
    }

    ptr = testList.head;
    while(ptr != NULL){
        //For every reference alarm, see if the test alarm list contains it
        Alarm* currTestAlarm = (Alarm*)ptr->data;
        if (!containsAlarm(refList, currTestAlarm)){
            return false;
        }
        ptr = ptr->next;
    }


    return true;
}

bool eventEqual(const Event* testEvent, const Event* refEvent){
    if (testEvent == NULL || refEvent == NULL){
        return false;
    }

    //Compare UID
    if (strcmp(testEvent->UID, testEvent->UID) != 0){

        return false;
    }
    //Compare creationDateTime
    if (!dtEqual(testEvent->creationDateTime, refEvent->creationDateTime)){

        return false;
    }

    //Compare property lists
    if (!pListEqual(testEvent->properties, refEvent->properties)){

        return false;
    }

    //Compare alarm lists
    if (!aListEqual(testEvent->alarms, refEvent->alarms)){

        return false;
    }

    return true;
}

bool containsEvent(List list, const Event* event) {
    Node *ptr = list.head;

    while(ptr != NULL) {

        Event* evnt = (Event*)ptr->data;
        if (eventEqual(evnt, event)) {
            return true;
        }
        ptr = ptr->next;
    }
    return false;
}

bool eListEqual(List testList, List refList) {

    Node* ptr = refList.head;
    while(ptr != NULL) {

        Event* evnt = (Event*)ptr->data;
        if (!containsEvent(testList, evnt)){
            //printf("Event Not Contained!\n");
            return false;
        }
        ptr = ptr->next;
    }

    ptr = testList.head;
    while(ptr != NULL) {
        Event* evnt = (Event*)ptr->data;
        if (!containsEvent(refList, evnt)){
            return false;
        }
        ptr = ptr->next;
    }

    return true;
}

bool calEqual(const Calendar* testCal, const Calendar* refCal){
    if (testCal == NULL || refCal == NULL){
        printf("Someone is NULL\n");
        return false;
    }

    //Compare version
    if (testCal->version != refCal->version){
        printf("Incorrect Version\n");
        return false;
    }

    //Compare prodID
    if (strcmp(testCal->prodID, refCal->prodID) != 0){
        printf("Incorrect prodID\n");
        return false;
    }

    //Compare events
    if (!eListEqual(testCal->events, refCal->events)){
        printf("Incorrect Events!\n");
        return false;
    }

    return true;
}

List calInitList(Calendar * cal) {
    List list;

    list.length = 0;
    list.tail = NULL;
    list.head = NULL;
    list.compare = cal->properties.compare;
    list.printData = cal->properties.printData;
    list.deleteData = cal->properties.deleteData;


    return list;
}

List alarmInitList(List lst) {
    List list;

    list.deleteData = lst.deleteData;
    list.printData = lst.printData;
    list.compare = lst.compare;
    list.head = NULL;
    list.tail = NULL;
    list.length = 0;
    return list;
}

List initWithExisting(List lst) {
    List list;

    list.deleteData = lst.deleteData;
    list.printData = lst.printData;
    list.compare = lst.compare;
    list.head = NULL;
    list.tail = NULL;
    list.length = 0;
    return list;
}



// MARK: A1 TEST CASES


//Calendar with no Event Alarms or Properties, non-UTC
Calendar* simpleCalnedar(void)
{
    Calendar* calendar = malloc(sizeof(Calendar));
    calendar->version = 2;
    strcpy(calendar->prodID,"-//hacksw/handcal//NONSGML v1.0//EN");

    Event *event = malloc(sizeof(Event));
    strcpy(event->UID, "uid1@example.com");
    strcpy(event->creationDateTime.date, "19970714");
    strcpy(event->creationDateTime.time, "170000");
    event->creationDateTime.UTC = false;

    strcpy(event->startDateTime.date, "19970714");
    strcpy(event->startDateTime.time, "170000");
    event->creationDateTime.UTC = false;

    event->properties =tInitializeList(NULL, NULL, NULL);
    event->properties =tInitializeList(NULL, NULL, NULL);



    return calendar;
}

//Calendar with no Event Alarms or Properties, UTC
Calendar* simpleCalnedarUTC(void)
{
    Calendar* calendar = malloc(sizeof(Calendar));
    calendar->version = 2;
    strcpy(calendar->prodID,"-//hacksw/handcal//NONSGML v1.0//EN");

    Event *event = malloc(sizeof(Event));
    strcpy(event->UID, "uid1@example.com");
    strcpy(event->creationDateTime.date, "19970714");
    strcpy(event->creationDateTime.time, "170000");
    event->creationDateTime.UTC = true;

    strcpy(event->startDateTime.date, "19970714");
    strcpy(event->startDateTime.time, "170000");
    event->creationDateTime.UTC = true;


    event->properties =tInitializeList(NULL, NULL, NULL);
    event->properties =tInitializeList(NULL, NULL, NULL);

    calendar->events =tInitializeList(NULL, NULL, NULL);
    tInsertBack(&calendar->events, event);

    return calendar;
}

//Calendar with Event Properties and no Alarms
Calendar* evtPropCalnedar(void)
{
    Calendar* calendar = malloc(sizeof(Calendar));
    calendar->version = 2;
    strcpy(calendar->prodID,"-//hacksw/handcal//NONSGML v1.0//EN");

    Event *event = malloc(sizeof(Event));
    strcpy(event->UID, "uid1@example.com");
    strcpy(event->creationDateTime.date, "19970714");
    strcpy(event->creationDateTime.time, "170000");
    event->creationDateTime.UTC = true;

    strcpy(event->startDateTime.date, "19970714");
    strcpy(event->startDateTime.time, "170000");
    event->creationDateTime.UTC = true;

    event->properties =tInitializeList(NULL, NULL, NULL);
    event->properties =tInitializeList(NULL, NULL, NULL);

    Property* prop = createTestProp("ORGANIZER", "CN=John Doe:MAILTO:john.doe@example.com");
    tInsertBack(&event->properties, (void*)prop);
    prop = createTestProp("DTEND", "19970715T035959Z");
    tInsertBack(&event->properties, (void*)prop);
    prop = createTestProp("CLASS", "PUBLIC");
    tInsertBack(&event->properties, (void*)prop);
    prop = createTestProp("SUMMARY", "Bastille Day Party");
    tInsertBack(&event->properties, (void*)prop);

    calendar->events =tInitializeList(NULL, NULL, NULL);
    tInsertBack(&calendar->events, event);

    return calendar;
}


//Calendar with Event Properties and Alarms
Calendar* almPropCalnedar(void)
{
    Calendar* calendar = malloc(sizeof(Calendar));
    calendar->version = 2.0;
    strcpy(calendar->prodID,"-//Mozilla.org/NONSGML Mozilla Calendar V1.1//EN");

    Event* event = malloc(sizeof(Event));


    strcpy(event->UID, "332414a0-54a1-408b-9cb1-2c9d1ad3696d");
    strcpy(event->creationDateTime.date, "20160106");
    strcpy(event->creationDateTime.time, "145812");
    event->creationDateTime.UTC = true;

    strcpy(event->startDateTime.date, "20151002");
    strcpy(event->startDateTime.time, "100000");
    event->creationDateTime.UTC = true;


    event->properties =tInitializeList(NULL, NULL, NULL);
    event->alarms =tInitializeList(NULL, NULL, NULL);

    //Add properties
    Property* prop = createTestProp("CREATED", "20160106T145812Z");
    tInsertBack(&event->properties, (void*)prop);
    prop = createTestProp("LAST-MODIFIED", "20160106T145812Z");
    tInsertBack(&event->properties, (void*)prop);
    prop = createTestProp("SUMMARY", "Han Solo @ Naboo");
    tInsertBack(&event->properties, (void*)prop);
    prop = createTestProp("STATUS", "CONFIRMED");
    tInsertBack(&event->properties, (void*)prop);
    prop = createTestProp("ORGANIZER", "CN=Obi-Wan Kenobi;mailto:laowaion@padawan.com");
    tInsertBack(&event->properties, (void*)prop);
    prop = createTestProp("DTEND", "20151002T110000");
    tInsertBack(&event->properties, (void*)prop);
//    prop = createTestProp("X-YAHOO-USER-STATUS", "BUSY");
//    tInsertBack(&event->properties, (void*)prop);

    //Add alarms
    //Alarm 1
    Alarm* testAlm = malloc(sizeof(Alarm));
    char tmpData[1000];

    strcpy(testAlm->action, "AUDIO");
    strcpy(tmpData, "VALUE=DATE-TIME:19970317T133000Z");
    testAlm->trigger = malloc(sizeof(char) * (strlen(tmpData)+1) );
    strcpy(testAlm->trigger, tmpData);

    testAlm->properties =tInitializeList(NULL, NULL, NULL);
    prop = createTestProp("REPEAT", "4");
    tInsertBack(&testAlm->properties, (void*)prop);
    prop = createTestProp("DURATION", "PT15M");
    tInsertBack(&testAlm->properties, (void*)prop);
    prop = createTestProp("ATTACH", "FMTTYPE=audio/basic:ftp://example.com/pub/sounds/bell-01.aud");
    tInsertBack(&testAlm->properties, (void*)prop);

    tInsertBack(&event->alarms, testAlm);

    //Alarm 2
    testAlm = malloc(sizeof(Alarm));

    strcpy(testAlm->action, "AUDIO");
    strcpy(tmpData, "VALUE=DATE-TIME:19970317T173000Z");
    testAlm->trigger = malloc(sizeof(char) * (strlen(tmpData)+1) );
    strcpy(testAlm->trigger, tmpData);

    testAlm->properties =tInitializeList(NULL, NULL, NULL);
    prop = createTestProp("REPEAT", "4");
    tInsertBack(&testAlm->properties, (void*)prop);
    prop = createTestProp("DURATION", "PT25M");
    tInsertBack(&testAlm->properties, (void*)prop);
    prop = createTestProp("ATTACH", "FMTTYPE=audio/basic:ftp://example.com/pub/sounds/bell-02.aud");
    tInsertBack(&testAlm->properties, (void*)prop);

    tInsertBack(&event->alarms, testAlm);

    calendar->events =tInitializeList(NULL, NULL, NULL);
    tInsertBack(&calendar->events, event);

    return calendar;
}

//******************************** TEST CASES ********************************
//Calendar creation - testCalSimpleNoUTC.ics
SubTestRec createCalTest1(int testNum, int subTest){
    SubTestRec result;
    char feedback[300];
    char fileName[] = "testFiles/validCalendar/testCalSimpleNoUTC.ics";

    Calendar* refCal = simpleCalnedar();
    Calendar* testCal;

    ICalErrorCode err = createCalendar(fileName, &testCal);

    if (err != OK){
        sprintf(feedback, "Subtest %d.%d: Did not return OK when parsing a valid file (%s).",testNum,subTest, fileName);
        result = createSubResult(FAIL, feedback);
        return result;
    }

    if (calEqual(testCal, refCal)){
        sprintf(feedback, "Subtest %d.%d: file %s parsed correctly",testNum,subTest,fileName);
        result = createSubResult(SUCCESS, feedback);
        return result;
    }else{
        sprintf(feedback, "Subtest %d.%d: did not correctly parse a valid file (%s)",testNum,subTest, fileName);
        result = createSubResult(FAIL, feedback);
        return result;
    }
}

//Calendar creation - testCalSimpleUTC.ics
SubTestRec createCalTest2(int testNum, int subTest){
    SubTestRec result;
    char feedback[300];
    char fileName[] = "testFiles/validCalendar/testCalSimpleUTC.ics";

    Calendar* refCal = simpleCalnedarUTC();
    Calendar* testCal;

    ICalErrorCode err = createCalendar(fileName, &testCal);

    if (err != OK){
        sprintf(feedback, "Subtest %d.%d: Did not return OK when parsing a valid file (%s).",testNum,subTest, fileName);
        result = createSubResult(FAIL, feedback);
        return result;
    }

    if (calEqual(testCal, refCal)){
        sprintf(feedback, "Subtest %d.%d: file %s parsed correctly",testNum,subTest,fileName);
        result = createSubResult(SUCCESS, feedback);
        return result;
    }else{
        sprintf(feedback, "Subtest %d.%d: did not correctly parse a valid file (%s)",testNum,subTest, fileName);
        result = createSubResult(FAIL, feedback);
        return result;
    }
}

//Calendar creation - testCalSimpleUTCComments.ics
SubTestRec createCalTest3(int testNum, int subTest){
    SubTestRec result;
    char feedback[300];
    char fileName[] = "testFiles/validCalendar/testCalSimpleUTCComments.ics";

    Calendar* refCal = simpleCalnedarUTC();
    Calendar* testCal;

    ICalErrorCode err = createCalendar(fileName, &testCal);

    if (err != OK){
        sprintf(feedback, "Subtest %d.%d: Did not return OK when parsing a valid file (%s).",testNum,subTest, fileName);
        result = createSubResult(FAIL, feedback);
        return result;
    }

    if (calEqual(testCal, refCal)){
        sprintf(feedback, "Subtest %d.%d: file %s parsed correctly",testNum,subTest,fileName);
        result = createSubResult(SUCCESS, feedback);
        return result;
    }else{
        sprintf(feedback, "Subtest %d.%d: did not correctly parse a valid file with comments(%s)",testNum,subTest, fileName);
        result = createSubResult(FAIL, feedback);
        return result;
    }
}

//Calendar creation - testCalEvtProp.ics
SubTestRec createCalTest4(int testNum, int subTest){
    SubTestRec result;
    char feedback[300];
    char fileName[] = "testFiles/validCalendar/testCalEvtProp.ics";

    Calendar* refCal = evtPropCalnedar();
    Calendar* testCal;

    ICalErrorCode err = createCalendar(fileName, &testCal);

    if (err != OK){
        sprintf(feedback, "Subtest %d.%d: Did not return OK when parsing a valid file (%s).",testNum,subTest, fileName);
        result = createSubResult(FAIL, feedback);
        return result;
    }

    if (calEqual(testCal, refCal)){
        sprintf(feedback, "Subtest %d.%d: file %s parsed correctly",testNum,subTest,fileName);
        result = createSubResult(SUCCESS, feedback);
        return result;
    }else{
        sprintf(feedback, "Subtest %d.%d: did not correctly parse a valid file (%s)",testNum,subTest, fileName);
        result = createSubResult(FAIL, feedback);
        return result;
    }
}

//Calendar creation - testCalEvtProp.ics
SubTestRec createCalTest5(int testNum, int subTest){
    SubTestRec result;
    char feedback[300];
    char fileName[] = "testFiles/validCalendar/testCalEvtPropAlm.ics";

    Calendar* refCal = almPropCalnedar();
    Calendar* testCal;

    ICalErrorCode err = createCalendar(fileName, &testCal);
    printf("Error is %s\n", errVals[err]);
    if (err != OK){
        sprintf(feedback, "Subtest %d.%d: Did not return OK when parsing a valid file (%s).",testNum,subTest, fileName);
        result = createSubResult(FAIL, feedback);
        return result;
    }

    if (calEqual(testCal, refCal)){
        sprintf(feedback, "Subtest %d.%d: file %s parsed correctly",testNum,subTest,fileName);
        result = createSubResult(SUCCESS, feedback);
        return result;
    }else{
        sprintf(feedback, "Subtest %d.%d: did not correctly parse a valid file (%s)",testNum,subTest, fileName);
        result = createSubResult(FAIL, feedback);
        return result;
    }
}

testRec* simpleCalTest(int testNum){
    const int numSubs = 2;  //doesn't need to be a variable but its a handy place to keep it
    int subTest = 1;
    char feedback[300];

    sprintf(feedback, "Test %d: Creating simple calendar objects from valid calendar files", testNum);
    testRec * rec = initRec(testNum, numSubs, feedback);

    //runSubTest(testNum, subTest, rec, &createCalTest1);
    //subTest++;
    runSubTest(testNum, subTest, rec, &createCalTest2);
    subTest++;
    runSubTest(testNum, subTest, rec, &createCalTest3);
    return rec;
}

testRec* medCalTest(int testNum){
    const int numSubs = 1;  //doesn't need to be a variable but its a handy place to keep it
    int subTest = 1;
    char feedback[300];

    sprintf(feedback, "Test %d: Creating a calendar object with event properties from a valid calendar file", testNum);
    testRec * rec = initRec(testNum, numSubs, feedback);

    runSubTest(testNum, subTest, rec, &createCalTest4);
    return rec;
}

testRec* largeCalTest(int testNum){
    const int numSubs = 1;  //doesn't need to be a variable but its a handy place to keep it
    int subTest = 1;
    char feedback[300];

    sprintf(feedback, "Test %d: Creating a calendar object with multiple alarms and event properties from a valid calendar file", testNum);
    testRec * rec = initRec(testNum, numSubs, feedback);

    runSubTest(testNum, subTest, rec, &createCalTest5);
    return rec;
}
//***************************************************************


//********************* Test printCalendar ***********************
//Printing creation - testCalSimpleUTCComments.ics
SubTestRec printCalTest1(int testNum, int subTest){
    SubTestRec result;
    char feedback[300];
    char fileName[] = "testFiles/validCalendar/testCalEvtPropAlm1.ics";

    Calendar* testCal;

    createCalendar(fileName, &testCal);

    char* calStr = printCalendar(testCal);
    fprintf(stderr, "%s\n", calStr);
    if(calStr != NULL)
    {
        sprintf(feedback, "Subtest %d.%d: Printed calendar.",testNum,subTest);
        result = createSubResult(SUCCESS, feedback);
        return result;
    }
    else
    {
        sprintf(feedback, "Subtest %d.%d: rintCalendar did not handle non-empty calendar correctly",testNum,subTest);
        result = createSubResult(FAIL, feedback);
        return result;
    }
    free(calStr);

}


SubTestRec printCalTest2(int testNum, int subTest){
    SubTestRec result;
    char feedback[300];

    char* calStr = printCalendar(NULL);

    if(calStr == NULL || calStr != NULL )
    {
        sprintf(feedback, "Subtest %d.%d: Empty calendar handled correctly.",testNum,subTest);
        result = createSubResult(SUCCESS, feedback);
        return result;
    }
    else
    {
        sprintf(feedback, "Subtest %d.%d: printCalendar did not handle empty calendar correctly",testNum,subTest);
        result = createSubResult(FAIL, feedback);
        return result;
    }
}

testRec* printCalTest(int testNum){
    const int numSubs = 2;  //doesn't need to be a variable but its a handy place to keep it
    int subTest = 1;
    char feedback[300];

    sprintf(feedback, "Test %d: Printing calendar", testNum);
    testRec * rec = initRec(testNum, numSubs, feedback);

    runSubTest(testNum, subTest, rec, &printCalTest1);
    subTest++;
    runSubTest(testNum, subTest, rec, &printCalTest2);
    return rec;
}
//***************************************************************




//********************* Test deleteClandar **********************
//Calendar deletion - testCalSimpleUTC.ics
SubTestRec deleteCalTest1(int testNum, int subTest){
    SubTestRec result;
    char feedback[300];
    char fileName[] = "testFiles/validCalendar/testCalSimpleUTC.ics";

    Calendar* testCal;
    createCalendar(fileName, &testCal);

    sprintf(feedback, "Subtest %d.%d: simple calendar deleted with no crashes",testNum,subTest);
    result = createSubResult(SUCCESS, feedback);
    return result;
}

//Calendar creation - testCalEvtProp.ics
SubTestRec deleteCalTest2(int testNum, int subTest){
    SubTestRec result;
    char feedback[300];
    char fileName[] = "testFiles/validCalendar/testCalEvtProp.ics";


    Calendar* testCal;
    createCalendar(fileName, &testCal);

    sprintf(feedback, "Subtest %d.%d: calendar with event properties deleted with no crashes",testNum,subTest);
    result = createSubResult(SUCCESS, feedback);
    return result;
}

//Calendar creation - testCalEvtProp.ics
SubTestRec deleteCalTest3(int testNum, int subTest){
    SubTestRec result;
    char feedback[300];
    char fileName[] = "testFiles/validCalendar/testCalEvtPropAlm1.ics";

    Calendar* testCal;
    createCalendar(fileName, &testCal);

    sprintf(feedback, "Subtest %d.%d: calendar with event properties and alarms deleted with no crashes",testNum,subTest);
    result = createSubResult(SUCCESS, feedback);
    return result;
}

testRec* deleteCalTest(int testNum){
    const int numSubs = 3;  //doesn't need to be a variable but its a handy place to keep it
    int subTest = 1;
    char feedback[300];

    sprintf(feedback, "Test %d: Calendar deletion.  NOTE: see valgrind output for deletion memory leak report", testNum);
    testRec * rec = initRec(testNum, numSubs, feedback);

    runSubTest(testNum, subTest, rec, &deleteCalTest1);
    subTest++;
    runSubTest(testNum, subTest, rec, &deleteCalTest2);
    subTest++;
    runSubTest(testNum, subTest, rec, &deleteCalTest3);
    return rec;
}
//***************************************************************


//********************** Test printError ************************

SubTestRec printErrorTest1(int testNum, int subTest){
    SubTestRec result;
    char feedback[300];

    for (int i = 0; i < 11; i++){
        char* errStr = (void*) printError(i);
        printf("String %s\n", errStr);
        if (errStr == NULL){
            sprintf(feedback, "Subtest %d.%d: printError returns NULL for error code %s",testNum,subTest,tPrintError(i));
            result = createSubResult(FAIL, feedback);
            return result;
        }

        if (strlen(errStr) == 0){
            sprintf(feedback, "Subtest %d.%d: printError returns an empty string for error code %s",testNum,subTest,tPrintError(i));
            result = createSubResult(FAIL, feedback);
            return result;
        }
    }

    sprintf(feedback, "Subtest %d.%d: printError test",testNum,subTest);
    result = createSubResult(SUCCESS, feedback);
    return result;
}

testRec* printErrTest(int testNum){
    const int numSubs = 1;  //doesn't need to be a variable but its a handy place to keep it
    int subTest = 1;
    char feedback[300];

    sprintf(feedback, "Test %d: Testing printError", testNum);
    testRec * rec = initRec(testNum, numSubs, feedback);

    runSubTest(testNum, subTest, rec, &printErrorTest1);
    return rec;
}

//***************************************************************

//*********** Error handling - invalid file ***********

//Non-existent file
SubTestRec invFileTest1(int testNum, int subTest){
    SubTestRec result;
    char feedback[300];
    Calendar* testCal;
    char fileName[] = "testFiles/iDoNotExist.ics";

    ICalErrorCode err = createCalendar(fileName, &testCal);

    if (err == INV_FILE){
        sprintf(feedback, "Subtest %d.%d: Reading non-existent file (%s) .",testNum,subTest, fileName);
        result = createSubResult(SUCCESS, feedback);
        return result;
    }else{
        sprintf(feedback, "Subtest %d.%d: incorrect error code for non-existent file (%s)",testNum,subTest, fileName);
        result = createSubResult(FAIL, feedback);
        return result;
    }
}

//Incorrect extension
SubTestRec invFileTest2(int testNum, int subTest){
    SubTestRec result;
    char feedback[300];
    Calendar* testCal;
    char fileName[] = "testFiles/validCalendar/testCalSimpleUTC.foo";

    ICalErrorCode err = createCalendar(fileName, &testCal);

    if (err == INV_FILE){
        sprintf(feedback, "Subtest %d.%d: incorrect file extension (%s).",testNum,subTest, fileName);
        result = createSubResult(SUCCESS, feedback);
        return result;
    }else{
        sprintf(feedback, "Subtest %d.%d: incorrect error code for file with incorrect extesnion(%s)",testNum,subTest, fileName);
        result = createSubResult(FAIL, feedback);
        return result;
    }
}

//Invalid line endings
SubTestRec invFileTest3(int testNum, int subTest){
    SubTestRec result;
    char feedback[300];
    Calendar* testCal;
    char fileName[] = "testFiles/invCalendar/invLineEnding.ics";

    ICalErrorCode err = createCalendar(fileName, &testCal);

    if (err == INV_FILE || err == INV_CAL){
        sprintf(feedback, "Subtest %d.%d: file with invalid line endings.",testNum,subTest);
        result = createSubResult(SUCCESS, feedback);
        return result;
    }else{
        sprintf(feedback, "Subtest %d.%d: incorrect error code (%s) for file with invalid line endings.",testNum,subTest, tPrintError(err));
        result = createSubResult(FAIL, feedback);
        return result;
    }
}

//Null name
SubTestRec invFileTest4(int testNum, int subTest){
    SubTestRec result;
    char feedback[300];
    Calendar* testCal;

    ICalErrorCode err = createCalendar(NULL, &testCal);

    if (err == INV_FILE){
        sprintf(feedback, "Subtest %d.%d: NULL file name.",testNum,subTest);
        result = createSubResult(SUCCESS, feedback);
        return result;
    }else{
        sprintf(feedback, "Subtest %d.%d: incorrect error code for NULL file name",testNum,subTest);
        result = createSubResult(FAIL, feedback);
        return result;
    }
}

testRec* invFileTest(int testNum){
    const int numSubs = 4;  //doesn't need to be a variable but its a handy place to keep it
    int subTest = 1;
    char feedback[300];

    sprintf(feedback, "Test %d: Creating a calendar object from an invalid file", testNum);
    testRec * rec = initRec(testNum, numSubs, feedback);

    runSubTest(testNum, subTest, rec, &invFileTest1);
    subTest++;
    runSubTest(testNum, subTest, rec, &invFileTest2);
    subTest++;
    runSubTest(testNum, subTest, rec, &invFileTest3);
    subTest++;
    runSubTest(testNum, subTest, rec, &invFileTest4);
    return rec;
}
//***************************************************************

//*********** Error handling - invalid calendar ***********
SubTestRec invCalTest1(int testNum, int subTest){
    SubTestRec result;
    char feedback[300];
    Calendar* testCal;
    char fileName[] = "testFiles/invCalendar/invCalObject.ics";

    ICalErrorCode err = createCalendar(fileName, &testCal);

    if (err == INV_CAL){
        sprintf(feedback, "Subtest %d.%d: correctly handled invalid calendar object.",testNum,subTest);
        result = createSubResult(SUCCESS, feedback);
        return result;
    }else{
        sprintf(feedback, "Subtest %d.%d: incorrect error %s code for invalid calendar object",testNum,subTest,tPrintError(err));
        result = createSubResult(FAIL, feedback);
        return result;
    }
}

SubTestRec invCalTest2(int testNum, int subTest){
    SubTestRec result;
    char feedback[300];
    Calendar* testCal;
    char fileName[] = "testFiles/invCalendar/missingEndCal.ics";

    ICalErrorCode err = createCalendar(fileName, &testCal);

    if (err == INV_CAL){
        sprintf(feedback, "Subtest %d.%d: correctly handled invalid calendar object (missing END:VCALENDAR).",testNum,subTest);
        result = createSubResult(SUCCESS, feedback);
        return result;
    }else{
        sprintf(feedback, "Subtest %d.%d: incorrect error code (%s) for invalid calendar object (missing END:VCALENDAR)",testNum,subTest, tPrintError(err));
        result = createSubResult(FAIL, feedback);
        return result;
    }
}

SubTestRec invCalTest3(int testNum, int subTest){
    SubTestRec result;
    char feedback[300];
    Calendar* testCal;
    char fileName[] = "testFiles/invCalendar/missingProdID.ics";

    ICalErrorCode err = createCalendar(fileName, &testCal);

    if (err == INV_CAL){
        sprintf(feedback, "Subtest %d.%d: correctly handled invalid calendar object (missing PRODID).",testNum,subTest);
        result = createSubResult(SUCCESS, feedback);
        return result;
    }else{
        sprintf(feedback, "Subtest %d.%d: incorrect error code (%s) for invalid calendar object (missing PRODID)",testNum,subTest, tPrintError(err));
        result = createSubResult(FAIL, feedback);
        return result;
    }
}

SubTestRec invCalTest4(int testNum, int subTest){
    SubTestRec result;
    char feedback[300];
    Calendar* testCal;
    char fileName[] = "testFiles/invCalendar/missingVER.ics";

    ICalErrorCode err = createCalendar(fileName, &testCal);

    if (err == INV_CAL){
        sprintf(feedback, "Subtest %d.%d: correctly handled invalid calendar object (missing VER).",testNum,subTest);
        result = createSubResult(SUCCESS, feedback);
        return result;
    }else{
        sprintf(feedback, "Subtest %d.%d: incorrect error code (%s) for invalid calendar object (missing VER)",testNum,subTest, tPrintError(err));
        result = createSubResult(FAIL, feedback);
        return result;
    }
}

SubTestRec invCalTest5(int testNum, int subTest){
    SubTestRec result;
    char feedback[300];
    Calendar* testCal;
    char fileName[] = "testFiles/invCalendar/dupPRODID.ics";

    ICalErrorCode err = createCalendar(fileName, &testCal);

    if (err == DUP_PRODID){
        sprintf(feedback, "Subtest %d.%d: correctly handled invalid calendar object (duplicate PRODID).",testNum,subTest);
        result = createSubResult(SUCCESS, feedback);
        return result;
    }else{
        sprintf(feedback, "Subtest %d.%d: incorrect error code (%s) for invalid calendar object (duplicate PRODID)",testNum,subTest,tPrintError(err));
        result = createSubResult(FAIL, feedback);
        return result;
    }
}

SubTestRec invCalTest6(int testNum, int subTest){
    SubTestRec result;
    char feedback[300];
    Calendar* testCal;
    char fileName[] = "testFiles/invCalendar/dupVER.ics";

    ICalErrorCode err = createCalendar(fileName, &testCal);

    if (err == DUP_VER){
        sprintf(feedback, "Subtest %d.%d: correctly handled invalid calendar object (duplicate VERSION).",testNum,subTest);
        result = createSubResult(SUCCESS, feedback);
        return result;
    }else{
        sprintf(feedback, "Subtest %d.%d: incorrect error code (%s) for invalid calendar object (duplicate VERSION)",testNum,subTest,tPrintError(err));
        result = createSubResult(FAIL, feedback);
        return result;
    }
}

SubTestRec invCalTest7(int testNum, int subTest){
    SubTestRec result;
    char feedback[300];
    Calendar* testCal;
    char fileName[] = "testFiles/invCalendar/malfPRODID.ics";

    ICalErrorCode err = createCalendar(fileName, &testCal);

    if (err == INV_PRODID){
        sprintf(feedback, "Subtest %d.%d: correctly handled invalid calendar object (invalid PRODID).",testNum,subTest);
        result = createSubResult(SUCCESS, feedback);
        return result;
    }else{
        sprintf(feedback, "Subtest %d.%d: incorrect error code (%s) for invalid calendar object (invalid PRODID)",testNum,subTest,tPrintError(err));
        result = createSubResult(FAIL, feedback);
        return result;
    }
}

SubTestRec invCalTest8(int testNum, int subTest){
    SubTestRec result;
    char feedback[300];
    Calendar* testCal;
    char fileName[] = "testFiles/invCalendar/malfVER.ics";

    ICalErrorCode err = createCalendar(fileName, &testCal);

    if (err == INV_VER){
        sprintf(feedback, "Subtest %d.%d: correctly handled invalid calendar object (invalid VERSION).",testNum,subTest);
        result = createSubResult(SUCCESS, feedback);
        return result;
    }else{
        sprintf(feedback, "Subtest %d.%d: incorrect error code (%s) for invalid calendar object (invalid VERSION)",testNum,subTest, tPrintError(err));
        result = createSubResult(FAIL, feedback);
        return result;
    }
}

SubTestRec invCalTest9(int testNum, int subTest){
    SubTestRec result;
    char feedback[300];
    Calendar* testCal;
    char fileName[] = "testFiles/invCalendar/noEVT.ics";

    ICalErrorCode err = createCalendar(fileName, &testCal);

    if (err == INV_CAL || err == INV_EVENT){
        sprintf(feedback, "Subtest %d.%d: correctly handled invalid calendar object (missing EVENT).",testNum,subTest);
        result = createSubResult(SUCCESS, feedback);
        return result;
    }else{
        sprintf(feedback, "Subtest %d.%d: incorrect error code (%s) for invalid calendar object (missing EVENT)",testNum,subTest, tPrintError(err));
        result = createSubResult(FAIL, feedback);
        return result;
    }
}

testRec* invCalTest(int testNum){
    const int numSubs = 9;  //doesn't need to be a variable but its a handy place to keep it
    int subTest = 1;
    char feedback[300];

    sprintf(feedback, "Test %d: Parsing a file with an invalid calendar object", testNum);
    testRec * rec = initRec(testNum, numSubs, feedback);

    runSubTest(testNum, subTest, rec, &invCalTest1);
    subTest++;
    runSubTest(testNum, subTest, rec, &invCalTest2);
    subTest++;
    runSubTest(testNum, subTest, rec, &invCalTest3);
    subTest++;
    runSubTest(testNum, subTest, rec, &invCalTest4);
    subTest++;
    runSubTest(testNum, subTest, rec, &invCalTest5);
    subTest++;
    runSubTest(testNum, subTest, rec, &invCalTest6);
    subTest++;
    runSubTest(testNum, subTest, rec, &invCalTest7);
    subTest++;
    runSubTest(testNum, subTest, rec, &invCalTest8);
    subTest++;
    runSubTest(testNum, subTest, rec, &invCalTest9);
    return rec;
}
//***************************************************************

//*********** Error handling - invalid event ***********
SubTestRec invEvtTest1(int testNum, int subTest){
    SubTestRec result;
    char feedback[300];
    Calendar* testCal;
    char fileName[] = "testFiles/invEvent/missingUID.ics";

    ICalErrorCode err = createCalendar(fileName, &testCal);

    if (err == INV_EVENT){
        sprintf(feedback, "Subtest %d.%d: correctly handled invalid event (missing UID).",testNum,subTest);
        result = createSubResult(SUCCESS, feedback);
        return result;
    }else{
        sprintf(feedback, "Subtest %d.%d: incorrect error code (%s) for invalid event (missing UID).",testNum,subTest,tPrintError(err));
        result = createSubResult(FAIL, feedback);
        return result;
    }
}

SubTestRec invEvtTest2(int testNum, int subTest){
    SubTestRec result;
    char feedback[300];
    Calendar* testCal;
    char fileName[] = "testFiles/invEvent/missingDTSTAMP.ics";

    ICalErrorCode err = createCalendar(fileName, &testCal);

    if (err == INV_EVENT || err == INV_CREATEDT){
        sprintf(feedback, "Subtest %d.%d: correctly handled invalid event (missing DTSTAMP).",testNum,subTest);
        result = createSubResult(SUCCESS, feedback);
        return result;
    }else{
        sprintf(feedback, "Subtest %d.%d: incorrect error code (%s) for invalid event (missing DTSTAMP).",testNum,subTest,tPrintError(err));
        result = createSubResult(FAIL, feedback);
        return result;
    }
}

SubTestRec invEvtTest3(int testNum, int subTest){
    SubTestRec result;
    char feedback[300];
    Calendar* testCal;
    char fileName[] = "testFiles/invEvent/invDT1.ics";

    ICalErrorCode err = createCalendar(fileName, &testCal);

    if (err == INV_CREATEDT){
        sprintf(feedback, "Subtest %d.%d: correctly handled malformed DTSTAMP (inv. date).",testNum,subTest);
        result = createSubResult(SUCCESS, feedback);
        return result;
    }else{
        sprintf(feedback, "Subtest %d.%d: incorrect error code (%s) for malformed DTSTAMP (inv. date).",testNum,subTest,tPrintError(err));
        result = createSubResult(FAIL, feedback);
        return result;
    }
}

SubTestRec invEvtTest4(int testNum, int subTest){
    SubTestRec result;
    char feedback[300];
    Calendar* testCal;
    char fileName[] = "testFiles/invEvent/invDT2.ics";

    ICalErrorCode err = createCalendar(fileName, &testCal);

    if (err == INV_CREATEDT){
        sprintf(feedback, "Subtest %d.%d: correctly handled malformed DTSTAMP (missing T separator).",testNum,subTest);
        result = createSubResult(SUCCESS, feedback);
        return result;
    }else{
        sprintf(feedback, "Subtest %d.%d: incorrect error code (%s) for malformed DTSTAMP (missing T separator).",testNum,subTest,tPrintError(err));
        result = createSubResult(FAIL, feedback);
        return result;
    }
}

SubTestRec invEvtTest5(int testNum, int subTest){
    SubTestRec result;
    char feedback[300];
    Calendar* testCal;
    char fileName[] = "testFiles/invEvent/invEvtProp1.ics";

    ICalErrorCode err = createCalendar(fileName, &testCal);

    if (err == INV_EVENT){
        sprintf(feedback, "Subtest %d.%d: correctly handled malformed event property (missing name).",testNum,subTest);
        result = createSubResult(SUCCESS, feedback);
        return result;
    }else{
        sprintf(feedback, "Subtest %d.%d: incorrect error code (%s) for malformed event property (missing name).",testNum,subTest,tPrintError(err));
        result = createSubResult(FAIL, feedback);
        return result;
    }
}

SubTestRec invEvtTest6(int testNum, int subTest){
    SubTestRec result;
    char feedback[300];
    Calendar* testCal;
    char fileName[] = "testFiles/invEvent/invEvtProp2.ics";

    ICalErrorCode err = createCalendar(fileName, &testCal);

    if (err == INV_EVENT){
        sprintf(feedback, "Subtest %d.%d: correctly handled malformed event property (missing descr).",testNum,subTest);
        result = createSubResult(SUCCESS, feedback);
        return result;
    }else{
        sprintf(feedback, "Subtest %d.%d: incorrect error code (%s) for malformed event property (missing descr).",testNum,subTest,tPrintError(err));
        result = createSubResult(FAIL, feedback);
        return result;
    }
}

testRec* invEvtTest(int testNum){
    const int numSubs = 6;  //doesn't need to be a variable but its a handy place to keep it
    int subTest = 1;
    char feedback[300];

    sprintf(feedback, "Test %d: Parsing a file with an invalid event", testNum);
    testRec * rec = initRec(testNum, numSubs, feedback);

    runSubTest(testNum, subTest, rec, &invEvtTest1);
    subTest++;
    runSubTest(testNum, subTest, rec, &invEvtTest2);
    subTest++;
    runSubTest(testNum, subTest, rec, &invEvtTest3);
    subTest++;
    runSubTest(testNum, subTest, rec, &invEvtTest4);
    subTest++;
    runSubTest(testNum, subTest, rec, &invEvtTest5);
    subTest++;
    runSubTest(testNum, subTest, rec, &invEvtTest6);

    return rec;
}
//***************************************************************


//*********** Error handling - invalid alarm ***********
SubTestRec invAlmTest1(int testNum, int subTest){
    SubTestRec result;
    char feedback[300];
    Calendar* testCal;
    char fileName[] = "testFiles/invAlarm/invAmlNoAction.ics";

    ICalErrorCode err = createCalendar(fileName, &testCal);

    if (err == INV_EVENT){
        sprintf(feedback, "Subtest %d.%d: correctly handled malformed alarm (missing action).",testNum,subTest);
        result = createSubResult(SUCCESS, feedback);
        return result;
    }else{
        sprintf(feedback, "Subtest %d.%d: incorrect error code (%s) for malformed malformed alarm (missing action).",testNum,subTest,tPrintError(err));
        result = createSubResult(FAIL, feedback);
        return result;
    }
}

SubTestRec invAlmTest2(int testNum, int subTest){
    SubTestRec result;
    char feedback[300];
    Calendar* testCal;
    char fileName[] = "testFiles/invAlarm/invAmlNoTrigger.ics";

    ICalErrorCode err = createCalendar(fileName, &testCal);

    if (err == INV_EVENT){
        sprintf(feedback, "Subtest %d.%d: correctly handled malformed alarm (missing trigger).",testNum,subTest);
        result = createSubResult(SUCCESS, feedback);
        return result;
    }else{
        sprintf(feedback, "Subtest %d.%d: incorrect error code (%s) for malformed malformed alarm (missing trigger).",testNum,subTest,tPrintError(err));
        result = createSubResult(FAIL, feedback);
        return result;
    }
}

SubTestRec invAlmTest3(int testNum, int subTest){
    SubTestRec result;
    char feedback[300];
    Calendar* testCal;
    char fileName[] = "testFiles/invAlarm/invAlmProp1.ics";

    ICalErrorCode err = createCalendar(fileName, &testCal);

    if (err == INV_ALARM){
        sprintf(feedback, "Subtest %d.%d: correctly handled malformed alarm property (missing name).",testNum,subTest);
        result = createSubResult(SUCCESS, feedback);
        return result;
    }else{
        sprintf(feedback, "Subtest %d.%d: incorrect error code (%s) for malformed alarm property property (missing name).",testNum,subTest,tPrintError(err));
        result = createSubResult(FAIL, feedback);
        return result;
    }
}

SubTestRec invAlmTest4(int testNum, int subTest){
    SubTestRec result;
    char feedback[300];
    Calendar* testCal;
    char fileName[] = "testFiles/invAlarm/invAlmProp2.ics";

    ICalErrorCode err = createCalendar(fileName, &testCal);

    if (err == INV_ALARM){
        sprintf(feedback, "Subtest %d.%d: correctly handled malformed alarm property (missing descr).",testNum,subTest);
        result = createSubResult(SUCCESS, feedback);
        return result;
    }else{
        sprintf(feedback, "Subtest %d.%d: incorrect error code (%s) for malformed alarm property (missing descr).",testNum,subTest,tPrintError(err));
        result = createSubResult(FAIL, feedback);
        return result;
    }
}

testRec* invAlmTest(int testNum){
    const int numSubs = 4;  //doesn't need to be a variable but its a handy place to keep it
    int subTest = 1;
    char feedback[300];

    sprintf(feedback, "Test %d: Parsing a file with an invalid alarm", testNum);
    testRec * rec = initRec(testNum, numSubs, feedback);

    runSubTest(testNum, subTest, rec, &invAlmTest1);
    subTest++;
    runSubTest(testNum, subTest, rec, &invAlmTest2);
    subTest++;
    runSubTest(testNum, subTest, rec, &invAlmTest3);
    subTest++;
    runSubTest(testNum, subTest, rec, &invAlmTest4);

    return rec;
}


// MARK: A2 TEST CASES

Calendar* multiEventCalendar(void) {

    Calendar* calendar = malloc(sizeof(Calendar));
    calendar->version = 2;
    strcpy(calendar->prodID, "//-//hacksw/handcal//NONSGML v1.0//EN");

    List eventList =tInitializeList(NULL, NULL, NULL);
    Event* firstEvent = malloc(sizeof(Event));
    strcpy(firstEvent->UID, "uid1@example.com");
    strcpy(firstEvent->creationDateTime.date, "19970714");
    strcpy(firstEvent->creationDateTime.time, "170000");

    Event* secondEvent = malloc(sizeof(Event));
    strcpy(secondEvent->UID, "uid1@example.com");
    strcpy(secondEvent->creationDateTime.date, "19970815");
    strcpy(secondEvent->creationDateTime.time, "170000");

    tInsertBack(&eventList, (void*)firstEvent);
    tInsertBack(&eventList, (void*)secondEvent);

    calendar->events = eventList;

    return calendar;
}

/**
 * Include two DTSTART properties
 * @return
 */
Calendar* invalidEventCalendar(Calendar* source) {
    Calendar* calendar = malloc(sizeof(Calendar));
    calendar->version = 2;
    strcpy(calendar->prodID, "//-//jtest/invalid/Event");

    calendar->properties = initWithExisting(source->properties);

    List eventList = initWithExisting(source->events);
    Event* firstEvent = malloc(sizeof(Event));
    strcpy(firstEvent->creationDateTime.date, "20160715");
    strcpy(firstEvent->creationDateTime.time, "170000");
    firstEvent->creationDateTime.UTC = true;
    Event *sourceEvent = (Event*)source->events.head;
    if (sourceEvent != NULL) {
        firstEvent->properties = initWithExisting(sourceEvent->properties);
    }
    Property *prop = createTestProp("DTSTART", "19970714T170000Z");
    tInsertBack(&firstEvent->properties, prop);

    prop = createTestProp("DTSTART", "19970714T170000Z");
    tInsertBack(&firstEvent->properties, prop);

    tInsertBack(&eventList,firstEvent);
    calendar->events = eventList;

    return calendar;
}

/**
 * Includes two PRODIDs and two DTSTARTs but should call INV_CAL
 * @return
 */
Calendar* invalidMultiComp(Calendar *source) {

    Calendar* calendar = malloc(sizeof(Calendar));
    calendar->version = 2;
    strcpy(calendar->prodID, "-//hacksw/handcal//NONSGML v1.0//EN");
    calendar->properties = initWithExisting(source->properties);

    Property* prop = createTestProp("PRODID", "ONLY/BE/ONE/OF/ME");
    tInsertBack(&calendar->properties, prop);
    Event * sourceEvent = (Event*)source->events.head;
    Event * event = malloc(sizeof(Event));

    strcpy(event->creationDateTime.date, "19970714");
    strcpy(event->creationDateTime.time, "170000");
    event->creationDateTime.UTC = true;

    if (sourceEvent != NULL) {
        event->properties = initWithExisting(sourceEvent->properties);
    } else {
        event->properties = tInitializeList(NULL, NULL, NULL);
    }


    prop = createTestProp("ORGANIZER", "CN=Jimmy Johnson:MAILTO:john.doe@example.com");
    tInsertBack(&event->properties, prop);
    prop = createTestProp("DTSTART", "19970714T170000Z");
    tInsertBack(&event->properties, prop);
    prop = createTestProp("DTEND", "19970715T035959Z");
    tInsertBack(&event->properties, prop);
    prop = createTestProp("DTSTART", "19970714T170000Z");
    tInsertBack(&event->properties, prop);
    prop = createTestProp("CLASS", "PUBLIC");
    tInsertBack(&event->properties, prop);
    prop = createTestProp("SUMMARY", "Lorem ipsum dolor sit amet, consectetur adipiscing elit.");
    tInsertBack(&event->properties, prop);

    calendar->events = initWithExisting(calendar->events);
    tInsertBack(&calendar->events, event);

    return calendar;

}

Calendar* invalidDuration(Calendar * source) {
    Calendar* calendar = malloc(sizeof(Calendar));
    calendar->version = 2;
    strcpy(calendar->prodID, "-//hacksw/handcal//NONSGML v1.0//EN");
    calendar->properties = initWithExisting(source->properties);

    Property* prop = NULL;

    Event * sourceEvent = (Event*)source->events.head;
    Event * event = malloc(sizeof(Event));

    strcpy(event->creationDateTime.date, "19970714");
    strcpy(event->creationDateTime.time, "170000");
    event->creationDateTime.UTC = true;
    strcpy(event->UID, "uid1@example.com");
    if (sourceEvent != NULL) {
        event->properties = initWithExisting(sourceEvent->properties);
    } else {
        sourceEvent = (Event*)source->events.tail;
        if (sourceEvent != NULL) {
            event->properties = initWithExisting(sourceEvent->properties);
        } else {
            event->properties = tInitializeList(NULL, NULL, NULL);
        }
    }


    prop = createTestProp("ORGANIZER", "CN=Jimmy Johnson:MAILTO:john.doe@example.com");
    tInsertBack(&event->properties, prop);
    prop = createTestProp("DTSTART", "19970714T170000Z");
    tInsertBack(&event->properties, prop);
    prop = createTestProp("DTEND", "19970715T035959Z");
    tInsertBack(&event->properties, prop);
    // should not have duration
    prop = createTestProp("DURATION", "60M");
    tInsertBack(&event->properties, prop);
    prop = createTestProp("CLASS", "PUBLIC");
    tInsertBack(&event->properties, prop);
    prop = createTestProp("SUMMARY", "Lorem ipsum dolor sit amet, consectetur adipiscing elit.");
    tInsertBack(&event->properties, prop);

    calendar->events = initWithExisting(source->events);
    tInsertBack(&calendar->events, event);
    Alarm *alm = NULL;
    if (sourceEvent != NULL) {
        alm = (Alarm *) sourceEvent->alarms.tail;
    }

    if (sourceEvent != NULL) {
        event->alarms = initWithExisting(sourceEvent->alarms);
    } else {
        event->alarms = tInitializeList(NULL, NULL, NULL);
    }


    Alarm *alarm = malloc(sizeof(Alarm));
    if (alm != NULL) {
        alarm->properties = initWithExisting(alarm->properties);
    } else {
        alm = (Alarm*)sourceEvent->alarms.head;
        if (alm != NULL) {
            alarm->properties = initWithExisting(alm->properties);
        } else {
            alarm->properties = tInitializeList(NULL, NULL, NULL);
        }
    }
    strcpy(alarm->action, "AUDIO");
    char tmpData[1000];
    strcpy(tmpData, "TRIGGER;VALUE=DATE-TIME:19970317T133000Z");
    alarm->trigger = malloc(sizeof(char) * strlen(tmpData));
    strcpy(alarm->trigger, tmpData);
    prop = createTestProp("TRIGGER", "VALUE=DATE-TIME:19970317T133000Z");
    tInsertBack(&alarm->properties, prop);

    tInsertBack(&event->alarms, alarm);

    return calendar;
}


Calendar* invalidCalScale(Calendar * source) {
    Calendar* calendar = malloc(sizeof(Calendar));
    calendar->version = 2;
    strcpy(calendar->prodID, "-//hacksw/handcal//NONSGML v1.0//EN");
    calendar->properties = initWithExisting(source->properties);

    Property* prop = NULL;

    Event * sourceEvent = (Event*)source->events.head;
    Event * event = malloc(sizeof(Event));

    strcpy(event->creationDateTime.date, "19970714");
    strcpy(event->creationDateTime.time, "170000");
    event->creationDateTime.UTC = true;
    strcpy(event->UID, "uid1@example.com");
    if (sourceEvent != NULL) {
        event->properties = initWithExisting(sourceEvent->properties);
    } else {
        sourceEvent = (Event*)source->events.tail;
        if (sourceEvent != NULL) {
            event->properties = initWithExisting(sourceEvent->properties);
        } else {
            event->properties = tInitializeList(NULL, NULL, NULL);
        }
    }

    prop = createTestProp("CALSCALE", "GREGORIAN");
    tInsertBack(&calendar->properties, prop);
    prop = createTestProp("CALSCALE", "GREGORIAN");
    tInsertBack(&calendar->properties, prop);

    prop = createTestProp("ORGANIZER", "CN=Jimmy Johnson:MAILTO:john.doe@example.com");
    tInsertBack(&event->properties, prop);
    prop = createTestProp("DTSTART", "19970714T170000Z");
    tInsertBack(&event->properties, prop);
    prop = createTestProp("DTEND", "19970715T035959Z");
    tInsertBack(&event->properties, prop);
    prop = createTestProp("CLASS", "PUBLIC");
    tInsertBack(&event->properties, prop);
    prop = createTestProp("SUMMARY", "Lorem ipsum dolor sit amet, consectetur adipiscing elit.");
    tInsertBack(&event->properties, prop);

    calendar->events = initWithExisting(source->events);
    tInsertBack(&calendar->events, event);


    return calendar;
}

Calendar* invalidAttach(Calendar *paramsCal) {
    Calendar* calendar = malloc(sizeof(Calendar));
    calendar->version = 2;
    strcpy(calendar->prodID, "-//hacksw/handcal//NONSGML v1.0//EN");
    calendar->properties = initWithExisting(paramsCal->properties);
    Property* prop = NULL;


    Event * event = malloc(sizeof(Event));
    //event->creationDateTime.time
    strcpy(event->creationDateTime.date, "19970714");
    strcpy(event->creationDateTime.time, "170000");
    event->creationDateTime.UTC = true;
    strcpy(event->UID, "uid1@example.com");

    Event * sourceEvent = (Event*)paramsCal->events.head;
    // Copy the event properties over
    if (sourceEvent != NULL) {
        event->properties = initWithExisting(sourceEvent->properties);
    } else {
        sourceEvent = (Event*)calendar->events.tail;
        if (sourceEvent != NULL) {
            event->properties = initWithExisting(sourceEvent->properties);
        } else {
            event->properties = tInitializeList(NULL, NULL, NULL);
        }
    }

    prop = createTestProp("ORGANIZER", "CN=Jimmy Johnson:MAILTO:john.doe@example.com");
    tInsertBack(&event->properties, prop);
    prop = createTestProp("DTSTART", "19970714T170000Z");
    tInsertBack(&event->properties, prop);
    prop = createTestProp("DTEND", "19970715T035959Z");
    tInsertBack(&event->properties, prop);
    prop = createTestProp("CLASS", "PUBLIC");
    tInsertBack(&event->properties, prop);
    prop = createTestProp("SUMMARY", "Lorem ipsum dolor sit amet, consectetur adipiscing elit.");
    tInsertBack(&event->properties, prop);

    calendar->events = initWithExisting(paramsCal->events);
    tInsertBack(&calendar->events, event);
    Alarm *sourceAlarm = NULL;
    if (sourceEvent != NULL) {
        sourceAlarm = (Alarm *) sourceEvent->alarms.head;

    if (sourceEvent != NULL) {
        event->alarms = initWithExisting(sourceEvent->alarms);
    } else {
        sourceEvent = (Event*)calendar->events.tail;
        if (sourceEvent != NULL) {
            event->alarms = initWithExisting(sourceEvent->alarms);
        } else {
            event->alarms = tInitializeList(NULL,NULL,NULL);
        }
    }
    } else {
        event->alarms = tInitializeList(NULL,NULL,NULL);
    }

    Alarm *alarm = malloc(sizeof(Alarm));

    if (sourceAlarm != NULL) {

        alarm->properties = initWithExisting(sourceAlarm->properties);

    } else {

        sourceAlarm = (Alarm*)sourceEvent->alarms.tail;

        if (sourceAlarm != NULL) {

            printf("We have checked it!\n");

            alarm->properties = initWithExisting(sourceAlarm->properties);
            printf("Alarm is not NULL now!\n");
        } else {
            alarm->properties = tInitializeList(NULL, NULL, NULL);

        }
    }




    strcpy(alarm->action, "AUDIO");
    char tmpData[1000];
    strcpy(tmpData, "TRIGGER;VALUE=DATE-TIME:19970317T133000Z");
    alarm->trigger = malloc(sizeof(char) * strlen(tmpData));
    strcpy(alarm->trigger, tmpData);
    // second trigger should be a problem!
    prop = createTestProp("TRIGGER", "VALUE=DATE-TIME:19970317T133000Z");
    tInsertBack(&alarm->properties, prop);

    tInsertBack(&event->alarms, alarm);

    return calendar;
}



Calendar* foldedCalendar1(void) {

    Calendar* calendar = malloc(sizeof(Calendar));
    strcpy(calendar->prodID, "-//hacksw/handcal//NONSGML v1.0//EN");
    calendar->version = 2.0;

    Event * event = malloc(sizeof(Event));

    strcpy(event->UID, "uid1@example.com");
    strcpy(event->creationDateTime.date, "19970714");
    strcpy(event->creationDateTime.time, "170000");
    event->creationDateTime.UTC = true;

    strcpy(event->startDateTime.date, "19970714");
    strcpy(event->startDateTime.time, "035959");
    event->creationDateTime.UTC = true;

    event->properties =tInitializeList(NULL, NULL, NULL);


    Property *prop = createTestProp("ORGANIZER", "CN=Jimmy Johnson:MAILTO:john.doe@example.com");
    tInsertBack(&event->properties, prop);
    prop = createTestProp("DTEND", "19970715T035959Z");
    tInsertBack(&event->properties, prop);
    prop = createTestProp("CLASS", "PUBLIC");
    tInsertBack(&event->properties, prop);
    prop = createTestProp("SUMMARY", "Lorem ipsum dolor sit amet, consectetur adipiscing elit.");
    tInsertBack(&event->properties, prop);

    calendar->events =tInitializeList(NULL, NULL, NULL);
    tInsertBack(&calendar->events, event);

    return calendar;
}

Calendar* foldedCalendar2(void) {

    Calendar* calendar = malloc((sizeof(Calendar)));
    strcpy(calendar->prodID, "-//hacksw/handcal//NONSGML v1.0//EN");
    calendar->version = 2.0;

    Event * event = malloc(sizeof(Event));
    strcpy(event->UID, "uid1@example.com");
    strcpy(event->creationDateTime.date, "19970714");
    strcpy(event->creationDateTime.time, "170000");
    event->creationDateTime.UTC = true;

    strcpy(event->creationDateTime.date, "19970714");
    strcpy(event->creationDateTime.time, "170000");
    event->creationDateTime.UTC = true;

    event->properties =tInitializeList(NULL, NULL, NULL);

    Property *prop = createTestProp("ORGANIZER", "CN=Jimmy Johnson:MAILTO:john.doe@example.com");
    tInsertBack(&event->properties, prop);
    prop = createTestProp("DTEND", "19970715T035959Z");
    tInsertBack(&event->properties, prop);
    prop = createTestProp("CLASS", "PUBLIC");
    tInsertBack(&event->properties, prop);
    prop = createTestProp("SUMMARY", "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.");
    tInsertBack(&event->properties, prop);
    calendar->events =tInitializeList(NULL, NULL, NULL);
    tInsertBack(&calendar->events, event);

    return calendar;
}


Calendar* foldedCalendar3(void) {

    Calendar * calendar = malloc((sizeof(Calendar)));
    strcpy(calendar->prodID, "-//Mozilla.org/NONSGML Mozilla Calendar V1.1//EN");

    calendar->version = 2.0;

    Property* prop = createTestProp("METHOD", "xyz");
    calendar->properties =tInitializeList(NULL, NULL, NULL);
    tInsertBack(&calendar->properties, prop);
    prop = createTestProp("CALSCALE", "GREGORIAN");
    tInsertBack(&calendar->properties, prop);


    Event* firstEvent = malloc(sizeof(Event));
    strcpy(firstEvent->UID, "332414a0-54a1-408b-9cb1-2c9d1ad3696d");
    strcpy(firstEvent->creationDateTime.date, "20160106");
    strcpy(firstEvent->creationDateTime.time, "145812");
    firstEvent->creationDateTime.UTC = true;

    strcpy(firstEvent->startDateTime.date, "20151002");
    strcpy(firstEvent->startDateTime.time, "100000");
    firstEvent->creationDateTime.UTC = true;


    firstEvent->properties =tInitializeList(NULL, NULL, NULL);
    prop = createTestProp("ORGANIZER", "CN=Obi-Wan Kenobi;mailto:laowaion@padawan.com");
    tInsertBack(&firstEvent->properties, prop);
    prop = createTestProp("LAST-MODIFIED", "20160106T145812Z");
    tInsertBack(&firstEvent->properties, prop);
    prop = createTestProp("CREATED", "20160106T145812Z");
    tInsertBack(&firstEvent->properties, prop);
    prop = createTestProp("SUMMARY", "Han Solo @ Naboo");
    tInsertBack(&firstEvent->properties, prop);
    prop = createTestProp("STATUS", "CONFIRMED");
    tInsertBack(&firstEvent->properties, prop);
    prop = createTestProp("DTEND", "20151002T110000");
    tInsertBack(&firstEvent->properties, prop);

    Alarm *alarmOne = malloc(sizeof(Alarm));
    char tmpData[1000];
    strcpy(tmpData, "VALUE=DATE-TIME:19970317T133000Z");
    alarmOne->trigger = malloc(sizeof(char) * (strlen(tmpData) + 1));
    strcpy(alarmOne->trigger, tmpData);
    strcpy(alarmOne->action, "AUDIO");


    alarmOne->properties =tInitializeList(NULL, NULL, NULL);
    prop = createTestProp("REPEAT", "4");
    tInsertBack(&alarmOne->properties, prop);
    prop = createTestProp("DURATION", "PT15M");
    tInsertBack(&alarmOne->properties, prop);
    prop = createTestProp("ATTACH", "FMTTYPE=audio/basic:ftp://example.com/pub/sounds/bell-01.aud");
    tInsertBack(&alarmOne->properties, prop);

    Alarm *alarmTwo = malloc(sizeof(Alarm));
    char data[1000];
    strcpy(tmpData, "-PT30M");
    alarmTwo->trigger = malloc(sizeof(char) * (strlen(data) +1));
    strcpy(alarmTwo->trigger, tmpData);
    strcpy(alarmTwo->action, "AUDIO");

    alarmTwo->properties =tInitializeList(NULL, NULL, NULL);
    prop = createTestProp("DURATION", "PT15M");
    tInsertBack(&alarmTwo->properties, prop);


    prop = createTestProp("REPEAT", "2");
    tInsertBack(&alarmTwo->properties, prop);

    firstEvent->alarms =tInitializeList(NULL, NULL, NULL);

    tInsertBack(&firstEvent->alarms, alarmOne);
    tInsertBack(&firstEvent->alarms, alarmTwo);

    calendar->events =tInitializeList(NULL, NULL, NULL);

    tInsertBack(&calendar->events, firstEvent);

    Event *secondEvent = malloc(sizeof(Event));
    strcpy(secondEvent->UID, "uid1@example.com");
    strcpy(secondEvent->UID, "332414a0-54a1-408b-9cb1-2c9d1ad3696d");
    strcpy(secondEvent->creationDateTime.date, "19970714");
    strcpy(secondEvent->creationDateTime.time, "170000");
    secondEvent->creationDateTime.UTC = true;


    strcpy(secondEvent->creationDateTime.date, "19970714");
    strcpy(secondEvent->creationDateTime.time, "170000");
    secondEvent->creationDateTime.UTC = true;

    secondEvent->properties =tInitializeList(NULL, NULL, NULL);

    prop = createTestProp("ORGANIZER", "CN=John Doe:MAILTO:john.doe@example.com");
    tInsertBack(&secondEvent->properties, prop);
    prop = createTestProp("DTEND", "19970715T035959Z");
    tInsertBack(&secondEvent->properties, prop);
    prop = createTestProp("CLASS", "PUBLIC");
    tInsertBack(&secondEvent->properties, prop);
    prop = createTestProp("SUMMARY", "Bastille Day Party");
    tInsertBack(&secondEvent->properties, prop);

    secondEvent->alarms =tInitializeList(NULL, NULL, NULL);
    tInsertBack(&secondEvent->alarms, alarmOne);
    tInsertBack(&secondEvent->alarms, alarmTwo);

    tInsertBack(&calendar->events, secondEvent);

    return calendar;

}

Calendar* megaCal() {

    Property *prop = NULL;
    Calendar* calendar = malloc((sizeof(Calendar)));
    strcpy(calendar->prodID, "-//hacksw/handcal//NONSGML v1.0//EN");
    calendar->version = 2.0;
    calendar->properties = tInitializeList(NULL, NULL, NULL);

    prop = createTestProp("CALSCALE", "GREGORIAN");
    tInsertBack(&calendar->properties, prop);

    // first event
    Event * event = malloc(sizeof(Event));
    strcpy(event->UID, "uid1@example.com");
    strcpy(event->creationDateTime.date, "19970714");
    strcpy(event->creationDateTime.time, "170000");
    event->creationDateTime.UTC = true;

    strcpy(event->startDateTime.date, "19970714");
    strcpy(event->startDateTime.time, "170000");
    event->startDateTime.UTC = true;


    event->properties =tInitializeList(NULL, NULL, NULL);

    prop = createTestProp("ORGANIZER", "CN=Jimmy Johnson:MAILTO:john.doe@example.com");
    tInsertBack(&event->properties, prop);
    prop = createTestProp("DTEND", "19970715T035959Z");
    tInsertBack(&event->properties, prop);
    prop = createTestProp("CLASS", "PUBLIC");
    tInsertBack(&event->properties, prop);
    prop = createTestProp("SUMMARY", "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.");
    tInsertBack(&event->properties, prop);

    calendar->events =tInitializeList(NULL, NULL, NULL);
    tInsertBack(&calendar->events, event);

    event->alarms = tInitializeList(NULL, NULL, NULL);

    Alarm* alarm1 = malloc(sizeof(Alarm));
    char tmpData[1000];
    strcpy(tmpData, "VALUE=DATE-TIME:19970317T133000Z");
    alarm1->trigger = malloc(sizeof(char) * (strlen(tmpData) +1));
    strcpy(alarm1->trigger, tmpData);
    strcpy(alarm1->action, "AUDIO");

    alarm1->properties = tInitializeList(NULL, NULL, NULL);
    prop = createTestProp("REPEAT", "5");
    tInsertBack(&alarm1->properties, prop);
    prop = createTestProp("DURATION", "PT55M");
    tInsertBack(&alarm1->properties, prop);
    prop = createTestProp("ATTACH", "FMTYPE=audio/basic:ftp://example.com/pub/sounds/really-longfile.aud");
    tInsertBack(&alarm1->properties, prop);

    tInsertBack(&event->alarms, alarm1);

    alarm1 = malloc(sizeof(Alarm));

    strcpy(tmpData, "VALUE=DATE-TIME:19970317T133000Z");
    alarm1->trigger = malloc(sizeof(char) * (strlen(tmpData) +1));
    strcpy(alarm1->trigger, tmpData);
    strcpy(alarm1->action, "AUDIO");

    alarm1->properties = tInitializeList(NULL, NULL, NULL);
    prop = createTestProp("REPEAT", "5");
    tInsertBack(&alarm1->properties, prop);
    prop = createTestProp("DURATION", "PT55M");
    tInsertBack(&alarm1->properties, prop);
    prop = createTestProp("ATTACH", "FMTYPE=audio/basic:ftp://example.com/pub/sounds/test.aud");
    tInsertBack(&alarm1->properties, prop);

    tInsertBack(&event->alarms, alarm1);
    tInsertBack(&event->alarms, alarm1); // 2 of the same alarms

    // second event

    event = malloc(sizeof(Event));
    strcpy(event->UID, "uid2@example.com");
    strcpy(event->creationDateTime.date, "19970714");
    strcpy(event->creationDateTime.time, "170000");
    event->creationDateTime.UTC = true;

    strcpy(event->startDateTime.date, "19970714");
    strcpy(event->startDateTime.time, "170000");
    event->startDateTime.UTC = true;

    event->properties =tInitializeList(NULL, NULL, NULL);

    prop = createTestProp("ORGANIZER", "CN=Cornelius Samuelson III:MAILTO:corneliussamsuelsoniii@gmail.com");
    tInsertBack(&event->properties, prop);

    prop = createTestProp("DTEND", "19970715T035959Z");
    tInsertBack(&event->properties, prop);

    prop = createTestProp("CLASS", "PUBLIC");
    tInsertBack(&event->properties, prop);
    prop = createTestProp("SUMMARY", "Lunch (Slop) at 12:30pm with the proletariat where Cornelius will discuss wage cuts.");
    tInsertBack(&event->properties, prop);
    prop = createTestProp("DESCRIPTION", "The wages are much too high and Cornelius requires an addition on his mansion to host parties. Such cost will be covered through extensive wage reductions.");
    tInsertBack(&event->properties, prop);

    tInsertBack(&calendar->events, event);

    // THIRD EVENT

    event = malloc(sizeof(Event));
    strcpy(event->UID, "uid1@example.com");
    strcpy(event->creationDateTime.date, "19970714");
    strcpy(event->creationDateTime.time, "170000");
    event->creationDateTime.UTC = true;

    strcpy(event->startDateTime.date, "19970714");
    strcpy(event->startDateTime.time, "170000");
    event->startDateTime.UTC = true;

    event->properties =tInitializeList(NULL, NULL, NULL);

    prop = createTestProp("ORGANIZER", "CN=Jimmy Johnson:MAILTO:john.doe@example.com");
    tInsertBack(&event->properties, prop);
    prop = createTestProp("DTEND", "19970715T035959Z");
    tInsertBack(&event->properties, prop);
    prop = createTestProp("CLASS", "PUBLIC");
    tInsertBack(&event->properties, prop);
    prop = createTestProp("PRIORITY", "7");
    tInsertBack(&event->properties, prop);
    prop = createTestProp("LOCATION", "SOMEWHEREVILLE");
    tInsertBack(&event->properties, prop);
    prop = createTestProp("GEO", "0.1241154;-122.08324");
    tInsertBack(&event->properties, prop);
    prop = createTestProp("STATUS", "CONFIRMED");
    tInsertBack(&event->properties, prop);
    prop = createTestProp("TRANSP", "OPAQUE");
    tInsertBack(&event->properties, prop);
    prop = createTestProp("RESOURCES", "Things that are necessary");
    tInsertBack(&event->properties, prop);
    prop = createTestProp("SUMMARY", "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.");
    tInsertBack(&event->properties, prop);
    prop = createTestProp("DESCRIPTION", "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Quisque porta accumsan ante, nec auctor risus fermentum in. Sed vestibulum pretium mi, vitae vestibulum mi auctor eu. Fusce mattis porta erat, non convallis nisi fringilla malesuada. Nulla odio massa, venenatis in est at, varius tempus ipsum. Vivamus molestie efficitur urna id ultricies. Aenean placerat pellentesque nunc sed iaculis. Suspendisse consequat lorem ligula, quis sagittis massa vestibulum ac. Cras dictum pretium enim, sit amet sollicitudin augue. Suspendisse nec neque auctor lectus sollicitudin vulputate. Integer non eleifend turpis. Maecenas at tellus feugiat, convallis enim a, pretium lorem. Ut vel enim sed magna porta pulvinar eu eu mi. Sed laoreet nibh arcu, eget tempus ex dapibus id. Aliquam interdum feugiat sapien vel tincidunt. Quisque elit sapien, malesuada at ultrices eget, tempus a erat. Nulla tristique tincidunt odio, et cursus ante gravida nec. Curabitur varius consequat nisi, eu varius turpis sagittis in. Vestibulum velit nisl, semper consectetur risus nec, ullamcorper volutpat est. Maecenas elementum euismod est pretium aliquam. Praesent feugiat sem mi, a viverra leo fermentum in. Fusce iaculis dictum eleifend. Maecenas sit amet ultricies augue. Vivamus sed mauris in orci bibendum egestas. Donec elementum a dolor in aliquet. Duis arcu augue, malesuada a odio a, bibendum pellentesque purus. Suspendisse urna magna, tristique vitae ipsum a, pellentesque pulvinar ante. Ut ut finibus turpis, eget eleifend odio.");
    tInsertBack(&event->properties, prop);

    alarm1 = malloc(sizeof(Alarm));
    strcpy(tmpData, "VALUE=DATE-TIME:19970317T133000Z");
    alarm1->trigger = malloc(sizeof(char) * (strlen(tmpData) +1));
    strcpy(alarm1->trigger, tmpData);
    strcpy(alarm1->action, "AUDIO");

    alarm1->properties = tInitializeList(NULL, NULL, NULL);
    prop = createTestProp("REPEAT", "5");
    tInsertBack(&alarm1->properties, prop);
    prop = createTestProp("DURATION", "PT55M");
    tInsertBack(&alarm1->properties, prop);
    prop = createTestProp("ATTACH", "FMTYPE=audio/basic:ftp://example.com/pub/sounds/really-longfile.aud");
    tInsertBack(&alarm1->properties, prop);

    event->alarms = tInitializeList(NULL, NULL, NULL);
    tInsertBack(&event->alarms, alarm1);

    tInsertBack(&calendar->events, event);

    return calendar;

}


Calendar* megaLineFolding() {
    char tmpData[1000];
    Property *prop = NULL;
    Calendar *cal = malloc(sizeof(Calendar));

    cal->events = tInitializeList(NULL, NULL, NULL);
    cal->version = 2.0;
    cal->properties = tInitializeList(NULL, NULL, NULL);
    strcpy(cal->prodID, "-/SOME/PROD/ID");

    prop = createTestProp("CALSCALE", "GREGORIAN");
    tInsertBack(&cal->properties, prop);
    prop = createTestProp("METHOD", "REQUEST");
    tInsertBack(&cal->properties, prop);

    // FIRST Event

    Event *event = malloc(sizeof(Event));
    strcpy(event->UID, "SOMEUID2@EXAMPLE.COM");
    strcpy(event->creationDateTime.date, "19970815");
    strcpy(event->creationDateTime.time, "170000");
    event->creationDateTime.UTC = true;
    event->properties = tInitializeList(NULL,NULL,NULL);
    event->alarms = tInitializeList(NULL,NULL,NULL);

    prop = createTestProp("CLASS", "PUBLIC");
    tInsertBack(&event->properties, prop);
    prop = createTestProp("DESCRIPTION", "Event description for a very big and exciting event.");
    tInsertBack(&event->properties, prop);
    prop = createTestProp("GEO", "-0.12451,17.017471");
    tInsertBack(&event->properties, prop);
    prop = createTestProp("LOCATION", "GUELPH");
    tInsertBack(&event->properties, prop);
    prop = createTestProp("ORGANIZER", "JDAWG@GMAIL.COM");
    tInsertBack(&event->properties, prop);
    prop = createTestProp("PRIORITY", "1000");
    tInsertBack(&event->properties, prop);
    prop = createTestProp("SEQUENCE", "1");
    tInsertBack(&event->properties, prop);
    prop = createTestProp("STATUS", "NEEDS-ACTION");
    tInsertBack(&event->properties, prop);
    prop = createTestProp("SUMMARY", "This event is not optional and you must attend.");
    tInsertBack(&event->properties, prop);
    prop = createTestProp("CATEGORIES", "BUSINESS,LEISURE,EVERYTHING");
    tInsertBack(&event->properties, prop);
    prop = createTestProp("TRANSP", "TRANSPARENT");
    tInsertBack(&event->properties, prop);

    // Add the Calendar
    tInsertBack(&cal->events, event);

    Alarm *alarm = malloc(sizeof(Alarm));
    alarm->properties = tInitializeList(NULL,NULL,NULL);
    strcpy(tmpData, "VALUE=DATE-TIME:19970317T133000Z");
    alarm->trigger = malloc(sizeof(char) * (strlen(tmpData) +1));
    strcpy(alarm->trigger, tmpData);
    strcpy(alarm->action, "AUDIO");


    prop = createTestProp("ATTACH", "FMTYPE=someaudio.aud");
    tInsertBack(&alarm->properties, prop);
    prop = createTestProp("DURATION", "50M");
    tInsertBack(&alarm->properties, prop);
    prop = createTestProp("REPEAT", "117");
    tInsertBack(&alarm->properties, prop);

    tInsertBack(&event->alarms, alarm);

    alarm = malloc(sizeof(Alarm));
    alarm->properties = tInitializeList(NULL,NULL,NULL);
    strcpy(tmpData, "VALUE=DATE-TIME:19970317T133000Z");
    alarm->trigger = malloc(sizeof(char) * (strlen(tmpData) +1));
    strcpy(alarm->trigger, tmpData);
    strcpy(alarm->action, "AUDIO");


    prop = createTestProp("ATTACH", "FMTYPE=longest/audiofile/nameever/to/test/youcan/handlethelinefoldings.aud");
    tInsertBack(&alarm->properties, prop);
    prop = createTestProp("DURATION", "50M");
    tInsertBack(&alarm->properties, prop);
    prop = createTestProp("REPEAT", "117");
    tInsertBack(&alarm->properties, prop);

    tInsertBack(&event->alarms, alarm);



    // Second Event

    event = malloc(sizeof(Event));
    strcpy(event->UID, "SOMEUID2@EXAMPLE.COM");
    strcpy(event->creationDateTime.date, "19970815");
    strcpy(event->creationDateTime.time, "170000");
    event->creationDateTime.UTC = true;
    event->properties = tInitializeList(NULL,NULL,NULL);
    event->alarms = tInitializeList(NULL,NULL,NULL);

    prop = createTestProp("CLASS", "PUBLIC");
    tInsertBack(&event->properties, prop);
    prop = createTestProp("DESCRIPTION", "Event description for a very big and exciting event.");
    tInsertBack(&event->properties, prop);
    prop = createTestProp("GEO", "-0.12451,17.017471");
    tInsertBack(&event->properties, prop);
    prop = createTestProp("LOCATION", "GUELPH");
    tInsertBack(&event->properties, prop);
    prop = createTestProp("ORGANIZER", "JDAWG@GMAIL.COM");
    tInsertBack(&event->properties, prop);
    prop = createTestProp("PRIORITY", "1000");
    tInsertBack(&event->properties, prop);
    prop = createTestProp("SEQUENCE", "1");
    tInsertBack(&event->properties, prop);
    prop = createTestProp("STATUS", "NEEDS-ACTION");
    tInsertBack(&event->properties, prop);
    prop = createTestProp("SUMMARY", "The summary with tons of line folding you should  be able to handle this   though.");
    tInsertBack(&event->properties, prop);
    prop = createTestProp("CATEGORIES", "BUSINESS,LEISURE,EVERYTHING");
    tInsertBack(&event->properties, prop);
    prop = createTestProp("TRANSP", "TRANSPARENT");
    tInsertBack(&event->properties, prop);

    // Add the Calendar
    tInsertBack(&cal->events, event);

    alarm = malloc(sizeof(Alarm));
    alarm->properties = tInitializeList(NULL,NULL,NULL);
    strcpy(tmpData, "VALUE=DATE-TIME:19970317T133000Z");
    alarm->trigger = malloc(sizeof(char) * (strlen(tmpData) +1));
    strcpy(alarm->trigger, tmpData);
    strcpy(alarm->action, "AUDIO");


    prop = createTestProp("ATTACH", "FMTYPE=someaudio.aud");
    tInsertBack(&alarm->properties, prop);
    prop = createTestProp("DURATION", "50M");
    tInsertBack(&alarm->properties, prop);
    prop = createTestProp("REPEAT", "117");
    tInsertBack(&alarm->properties, prop);

    tInsertBack(&event->alarms, alarm);

    alarm = malloc(sizeof(Alarm));
    alarm->properties = tInitializeList(NULL,NULL,NULL);
    strcpy(tmpData, "VALUE=DATE-TIME:19970317T133000Z");
    alarm->trigger = malloc(sizeof(char) * (strlen(tmpData) +1));
    strcpy(alarm->trigger, tmpData);
    strcpy(alarm->action, "AUDIO");


    prop = createTestProp("ATTACH", "FMTYPE=longest/audiofile/nameever/to/test/youcan/handlethelinefoldings.aud");
    tInsertBack(&alarm->properties, prop);
    prop = createTestProp("DURATION", "50M");
    tInsertBack(&alarm->properties, prop);
    prop = createTestProp("REPEAT", "117");
    tInsertBack(&alarm->properties, prop);

    tInsertBack(&event->alarms, alarm);

    // Third Event

    event = malloc(sizeof(Event));
    strcpy(event->UID, "SOMEUID2@EXAMPLE.COM");
    strcpy(event->creationDateTime.date, "19970815");
    strcpy(event->creationDateTime.time, "170000");
    event->creationDateTime.UTC = true;
    event->properties = tInitializeList(NULL,NULL,NULL);
    event->alarms = tInitializeList(NULL,NULL,NULL);

    prop = createTestProp("CLASS", "PUBLIC");
    tInsertBack(&event->properties, prop);
    prop = createTestProp("DESCRIPTION", "Event description.");
    tInsertBack(&event->properties, prop);
    prop = createTestProp("GEO", "-0.12451,17.017471");
    tInsertBack(&event->properties, prop);
    prop = createTestProp("LOCATION", "GUELPH");
    tInsertBack(&event->properties, prop);
    prop = createTestProp("ORGANIZER", "JDAWG@GMAIL.COM");
    tInsertBack(&event->properties, prop);
    prop = createTestProp("PRIORITY", "1000");
    tInsertBack(&event->properties, prop);
    prop = createTestProp("SEQUENCE", "1");
    tInsertBack(&event->properties, prop);
    prop = createTestProp("STATUS", "NEEDS-ACTION");
    tInsertBack(&event->properties, prop);
    prop = createTestProp("SUMMARY", "This event is not optional.");
    tInsertBack(&event->properties, prop);
    prop = createTestProp("CATEGORIES", "BUSINESS,LEISURE,EVERYTHING");
    tInsertBack(&event->properties, prop);
    prop = createTestProp("TRANSP", "TRANSPARENT");
    tInsertBack(&event->properties, prop);

    tInsertBack(&cal->events, event);

    return cal;

}


SubTestRec lineFoldingTest1(int testNum, int subTest) {
    SubTestRec result;
    char feedback[300];
    char filename[] = "testFiles/validA2/mLineProp1.ics";

    Calendar *refCal = foldedCalendar1();
    Calendar *testCal;

    ICalErrorCode err = createCalendar(filename, &testCal);

    if (err != OK) {
        sprintf(feedback, "Subtest %d.%d: Did not return OK when parsing a valid file (%s).", testNum, subTest, filename);
        result = createSubResult(FAIL, feedback);
        return result;
    }

    if (calEqual(testCal, refCal)) {
        sprintf(feedback, "Subtest %d.%d: file %s parsed correctly",testNum,subTest,filename);
        result = createSubResult(SUCCESS, feedback);
        return result;
    }else{
        sprintf(feedback, "Subtest %d.%d: did not correctly parse a valid file (%s)",testNum,subTest, filename);
        result = createSubResult(FAIL, feedback);
        return result;
    }

}

SubTestRec lineFoldingTest2(int testNum, int subTest) {
    SubTestRec result;
    char feedback[300];
    char filename[] = "testFiles/validA2/mLinePropTab1.ics";

    Calendar *refCal = foldedCalendar1();
    Calendar *testCal;

    ICalErrorCode err = createCalendar(filename, &testCal);

    if (err != OK) {
        sprintf(feedback, "Subtest %d.%d: Did not return OK when parsing a valid file (%s).", testNum, subTest, filename);
        result = createSubResult(FAIL, feedback);
        return result;
    }

    if (calEqual(testCal, refCal)) {
        sprintf(feedback, "Subtest %d.%d: file %s parsed correctly",testNum,subTest,filename);
        result = createSubResult(SUCCESS, feedback);
        return result;
    }else{
        sprintf(feedback, "Subtest %d.%d: did not correctly parse a valid file (%s)",testNum,subTest, filename);
        result = createSubResult(FAIL, feedback);
        return result;
    }

}

SubTestRec lineFoldingTest3(int testNum, int subTest) {
    SubTestRec result;
    char feedback[300];
    char filename[] = "testFiles/validCalendar/testCalEvtPropAlm3.ics";

    Calendar *refCal = foldedCalendar3();
    Calendar *testCal;

    ICalErrorCode err = createCalendar(filename, &testCal);

    if (err != OK) {
        sprintf(feedback, "Subtest %d.%d: Did not return OK when parsing a valid file (%s).", testNum, subTest, filename);
        result = createSubResult(FAIL, feedback);
        return result;
    }

    if (calEqual(testCal, refCal)) {
        sprintf(feedback, "Subtest %d.%d: file %s parsed correctly",testNum,subTest,filename);
        result = createSubResult(SUCCESS, feedback);
        return result;
    }else{
        sprintf(feedback, "Subtest %d.%d: did not correctly parse a valid file (%s)",testNum,subTest, filename);
        result = createSubResult(FAIL, feedback);
        return result;
    }

}

SubTestRec lineFoldingTest4(int testNum, int subTest) {
    SubTestRec result;
    char feedback[300];
    char filename[] = "testFiles/validA2/mLineTab1.ics";

    Calendar *refCal = foldedCalendar2();
    Calendar *testCal;

    ICalErrorCode err = createCalendar(filename, &testCal);

    if (err != OK) {
        sprintf(feedback, "Subtest %d.%d: Did not return OK when parsing a valid file (%s).", testNum, subTest, filename);
        result = createSubResult(FAIL, feedback);
        return result;
    }

    if (calEqual(testCal, refCal)) {
        sprintf(feedback, "Subtest %d.%d: file %s parsed correctly",testNum,subTest,filename);
        result = createSubResult(SUCCESS, feedback);
        return result;
    }else{
        sprintf(feedback, "Subtest %d.%d: did not correctly parse a valid file (%s)",testNum,subTest, filename);
        result = createSubResult(FAIL, feedback);
        return result;
    }

}


testRec* foldedCalTest(int testNum) {
    const int numSubs = 4;
    int subTest = 1;
    char feedback[300];

    sprintf(feedback, "Test %d: Creating simple folded calendar objects from valid calendar files (10%%)", testNum);
    testRec* rec = initRec(testNum, numSubs, feedback);


    runSubTest(testNum, subTest, rec, &lineFoldingTest1);
    subTest++;

    runSubTest(testNum, subTest, rec, &lineFoldingTest2);
    subTest++;

    runSubTest(testNum, subTest, rec, &lineFoldingTest3);
    subTest++;

    runSubTest(testNum, subTest, rec, &lineFoldingTest4);
    return rec;

}


SubTestRec invalidParams(int testNum, int subTest) {
    SubTestRec result;
    char feedback[300];
    char filename[] = "testFiles/invalidA2/XParams1.ics";

    Calendar* calendar;

    ICalErrorCode err = createCalendar(filename, &calendar);
    if (err == INV_CAL) {

        sprintf(feedback, "Subtest %d.%d: file %s error found",testNum,subTest,filename);
        result = createSubResult(SUCCESS, feedback);
        return result;
    } else {

        sprintf(feedback, "Subtest %d.%d: Did not return Invalid Event when parsing an invalid file (%s).", testNum, subTest, filename);
        result = createSubResult(FAIL, feedback);
        return result;
    }

}

SubTestRec invalidAlarm(int testNum, int subTest) {
    SubTestRec result;
    char feedback[300];
    char filename[] = "testFiles/invalidA2/invAlarm1.ics";

    Calendar *calendar;

    ICalErrorCode err = createCalendar(filename, &calendar);

    if (err == INV_ALARM) {

        sprintf(feedback, "Subtest %d.%d: file %s error found",testNum,subTest,filename);
        result = createSubResult(SUCCESS, feedback);
        return result;
    } else {

        sprintf(feedback, "Subtest %d.%d: Did not return Invalid Alarm when parsing an invalid file (%s) Returned (%d).", testNum, subTest, filename, err);
        result = createSubResult(FAIL, feedback);
        return result;
    }

}

SubTestRec invalidAlarmDup(int testNum, int subTest) {
    SubTestRec result;
    char feedback[300];
    char filename[] = "testFiles/invalidA2/invAlarmDupProps1.ics";

    Calendar *calendar;

    ICalErrorCode err = createCalendar(filename, &calendar);

    if (err == INV_ALARM) {
        sprintf(feedback, "Subtest %d.%d: file %s Duplicate Alarm property found", testNum, subTest, filename);
        result = createSubResult(SUCCESS, feedback);
        return result;
    } else {
        sprintf(feedback, "Subtest %d.%d: Did not return Invalid Alarm when parsing an invalid file (%s) Returned (%d).", testNum, subTest, filename, err);
        result = createSubResult(FAIL, feedback);
        return result;
    }
}

SubTestRec invalidDupProdID(int testNum, int subTest) {
    SubTestRec result;
    char feedback[300];
    char filename[] = "testFiles/invCalendar/dupPRODID.ics";

    Calendar *calendar;

    ICalErrorCode err = createCalendar(filename, &calendar);

    if (err == DUP_PRODID) {

        sprintf(feedback, "Subtest %d.%d: file %s duplicate PRODID found",testNum,subTest,filename);
        result = createSubResult(SUCCESS, feedback);
        return result;
    } else {
        sprintf(feedback, "Subtest %d.%d: Did not return DUP_PRODID when parsing an invalid file (%s).", testNum, subTest, filename);
        result = createSubResult(FAIL, feedback);
        return result;
    }

}

SubTestRec invalidRandomBegin(int testNum, int subTest) {
    SubTestRec result;
    char feedback[300];
    char filename[] = "testFiles/invalidA2/randomBegin1.ics";

    Calendar *calendar;

    ICalErrorCode err = createCalendar(filename, &calendar);

    if (err == INV_EVENT) {
        sprintf(feedback, "Subtest %d.%d: file %s invalid Event found",testNum,subTest,filename);
        result = createSubResult(SUCCESS, feedback);
        return result;
    } else {
        sprintf(feedback, "Subtest %d.%d: Did not return INV_EVENT when parsing an invalid file (%s) returned (%s).", testNum, subTest, filename, printError(err));
        result = createSubResult(FAIL, feedback);
        return result;
    }
}

SubTestRec duplicateCalScale(int testNum, int subTest) {
    SubTestRec result;
    char feedback[300];
    char filename[] = "testFiles/invalidA2/doubleCalScale1.ics";

    Calendar *calendar;

    ICalErrorCode err = createCalendar(filename, &calendar);

    if (err == INV_CAL) {
        sprintf(feedback, "Subtest %d.%d: file %s invalid Calscale found",testNum,subTest,filename);
        result = createSubResult(SUCCESS, feedback);
        return result;
    } else {
        sprintf(feedback, "Subtest %d.%d: Did not return INV_CAL when parsing an invalid file (%s).", testNum, subTest, filename);
        result = createSubResult(FAIL, feedback);
        return result;
    }

}

SubTestRec invalidDurationEnd(int testNum, int subTest) {
    SubTestRec result;
    char feedback[300];
    char filename[] = "testFiles/invalidA2/dtendduration1.ics";

    Calendar *calendar;

    ICalErrorCode err = createCalendar(filename, &calendar);

    if (err == INV_EVENT) {
        sprintf(feedback, "Subtest %d.%d: file %s invalid Calscale found",testNum,subTest,filename);
        result = createSubResult(SUCCESS, feedback);
        return result;
    } else {
        sprintf(feedback, "Subtest %d.%d: Did not return INV_EVENT when parsing an invalid file (%s) returned (%s).", testNum, subTest, filename, printError(err));
        result = createSubResult(FAIL, feedback);
        return result;
    }
}

testRec* paramsTest(int testNum) {
    const int numSubs = 7;
    int subTest = 1;
    char feedback[300];

    sprintf(feedback, "Test %d: Testing invalid Calendar Params (15%%)", testNum);
    testRec* rec = initRec(testNum, numSubs, feedback);


    runSubTest(testNum, subTest, rec, &invalidParams);
    subTest++;

    runSubTest(testNum, subTest, rec, &invalidAlarm);
    subTest++;

    runSubTest(testNum, subTest, rec, &invalidDupProdID);
    subTest++;

    runSubTest(testNum, subTest, rec, &invalidAlarmDup);
    subTest++;

    runSubTest(testNum, subTest, rec, &invalidRandomBegin);
    subTest++;

    runSubTest(testNum, subTest, rec, &duplicateCalScale);
    subTest++;

    runSubTest(testNum, subTest, rec, &invalidDurationEnd);

    return rec;

}

SubTestRec invalidEventCalendarValidate(int testNum, int subTest) {
    SubTestRec result;
    char feedback[300];
    char sourceParamsCal[] = "testFiles/validCalendar/sourceCal1.ics";
    Calendar *paramsCal = NULL;

    ICalErrorCode sourceErr = createCalendar(sourceParamsCal, &paramsCal);
    if (sourceErr != OK) {
        sprintf(feedback, "Subtest %d.%d: Did not parse valid source Calendar (returned %s)", testNum, subTest, printError(sourceErr));
        result = createSubResult(FAIL, feedback);
        return result;
    }

    Calendar* calendar = invalidEventCalendar(paramsCal);
    fprintf(stderr, "invalidEventCalendarValidate\n");
    ICalErrorCode err = validateCalendar(calendar);

    if (err == INV_EVENT) {
        sprintf(feedback, "Subtest %d.%d: invalid event successfully detected!", testNum, subTest);
        result = createSubResult(SUCCESS, feedback);
    } else {
        sprintf(feedback, "Subtest %d.%d: invalid event NOT detected (returned %s)!", testNum, subTest, printError(err));
        result = createSubResult(FAIL, feedback);
    }

    return result;

}

SubTestRec invalidMultiCompValidate(int testNum, int subTest) {
    SubTestRec result;
    char feedback[300];
    char sourceParamsCal[] = "testFiles/validCalendar/sourceCal1.ics";
    Calendar *paramsCal = NULL;

    ICalErrorCode sourceErr = createCalendar(sourceParamsCal, &paramsCal);
    if (sourceErr != OK) {
        sprintf(feedback, "Subtest %d.%d: Did not parse valid source Calendar (returned %s)", testNum, subTest, printError(sourceErr));
        result = createSubResult(FAIL, feedback);
        return result;
    }

    fprintf(stderr, "invalidMultiCompValidate\n");
    Calendar *calendar = invalidMultiComp(paramsCal);

    ICalErrorCode err = validateCalendar(calendar);

    if (err == DUP_PRODID || err == INV_CAL) {
        sprintf(feedback, "Subtest %d.%d: invalid calendar detected!", testNum, subTest);
        result = createSubResult(SUCCESS, feedback);
    } else {
        sprintf(feedback, "Subtest %d.%d: DUP_PRODID not detected! (returned %s)", testNum, subTest, printError(err));
        result = createSubResult(FAIL, feedback);
    }

    return result;
}

SubTestRec invalidDurationTest(int testNum, int subTest) {
    SubTestRec result;
    char feedback[300];
    char sourceParamsCal[] = "testFiles/validCalendar/sourceCal1.ics";
    Calendar *paramsCal = NULL;

    ICalErrorCode sourceErr = createCalendar(sourceParamsCal, &paramsCal);
    if (sourceErr != OK) {
        sprintf(feedback, "Subtest %d.%d: Did not parse valid source Calendar (returned %s)", testNum, subTest, printError(sourceErr));
        result = createSubResult(FAIL, feedback);
        return result;
    }
    Calendar* cal = invalidDuration(paramsCal);

    ICalErrorCode err = validateCalendar(cal);

    if (err == INV_EVENT) {
        sprintf(feedback, "Subtest %d.%d: invalid event detected!", testNum, subTest);
        result = createSubResult(SUCCESS, feedback);
    } else {
        sprintf(feedback, "Subtest %d.%d: invalid event not detected! (returned %s)", testNum, subTest, printError(err));
        result = createSubResult(FAIL, feedback);
    }

    return result;
}

SubTestRec invalidAttachTest(int testNum, int subTest) {
    SubTestRec result;
    char feedback[300];
    char sourceParamsCal[] = "testFiles/validCalendar/sourceCal1.ics";

    Calendar *paramsCal = NULL;


    ICalErrorCode sourceErr = createCalendar(sourceParamsCal, &paramsCal);
    if (sourceErr != OK) {
        sprintf(feedback, "Subtest %d.%d: Did not parse valid source Calendar (returned %s)", testNum, subTest, printError(sourceErr));
        result = createSubResult(FAIL, feedback);
        return result;
    }

    Calendar* cal = invalidAttach(paramsCal);
    ICalErrorCode err = validateCalendar(cal);

    if (err == INV_ALARM) {
        sprintf(feedback, "Subtest %d.%d: invalid alarm detected!", testNum, subTest);
        result = createSubResult(SUCCESS, feedback);
    } else {
        sprintf(feedback, "Subtest %d.%d: invalid alarm not detected! (returned %s)", testNum, subTest, printError(err));
        result = createSubResult(FAIL, feedback);
    }

    return result;
}

SubTestRec invalidCalScaleTest(int testNum, int subTest) {
    SubTestRec result;
    char feedback[300];
    char sourceParamsCal[] = "testFiles/validCalendar/sourceCal1.ics";

    Calendar *paramsCal = NULL;

    ICalErrorCode sourceErr = createCalendar(sourceParamsCal, &paramsCal);
    if (sourceErr != OK) {
        sprintf(feedback, "Subtest %d.%d: Did not parse valid source Calendar (returned %s)", testNum, subTest, printError(sourceErr));
        result = createSubResult(FAIL, feedback);
        return result;
    }

    Calendar* cal = invalidCalScale(paramsCal);
    printf("%s\n", "YOWE STARTIN");
    ICalErrorCode err = validateCalendar(cal);
    printf("%s\n", "WE MADE IT THROIGHT THE WHO THANG");

    if (err == INV_CAL) {
        sprintf(feedback, "Subtest %d.%d invalid calendar detected", testNum, subTest);
        result = createSubResult(SUCCESS, feedback);
    } else {
        sprintf(feedback, "Subtest %d.%d: invalid alarm not detected! (returned %s)", testNum, subTest, printError(err));
        result = createSubResult(FAIL, feedback);
    }

    return result;
}

testRec* validateTest(int testNum) {
    const int numSubs = 4;
    int subTest = 1;
    char feedback[300];

    sprintf(feedback, "Test %d: Testing Invalid Calendar Objects using validate() (15%%)", testNum);
    testRec* rec = initRec(testNum, numSubs, feedback);


    runSubTest(testNum, subTest, rec, &invalidEventCalendarValidate);
    subTest++;

    runSubTest(testNum, subTest, rec, &invalidMultiCompValidate);
    subTest++;

    runSubTest(testNum, subTest, rec, &invalidDurationTest);
    subTest++;

    //runSubTest(testNum, subTest, rec, &invalidAttachTest);
    //subTest++;

    runSubTest(testNum, subTest, rec, &invalidCalScaleTest);

    return rec;
}


SubTestRec outputTest1(int testNum, int subTest) {
    SubTestRec result;
    char feedback[300];
    char filename[] = "studentOutput/foldedCalendar1.ics";
    Calendar *testCal = foldedCalendar1();

    writeCalendar(filename, testCal);
    Calendar *refCal;
    ICalErrorCode err = createCalendar("studentOutput/foldedCalendar1.ics", &refCal);

    if (err != OK) {
        sprintf(feedback, "Subtest %d.%d: Did not return OK when parsing a valid file (%s).", testNum, subTest, filename);
        result = createSubResult(FAIL, feedback);
        return result;
    }

    if (calEqual(testCal, refCal)) {
        sprintf(feedback, "Subtest %d.%d: file %s parsed correctly",testNum,subTest,filename);
        result = createSubResult(SUCCESS, feedback);
        return result;
    }else{
        sprintf(feedback, "Subtest %d.%d: did not correctly parse a valid file (%s)",testNum,subTest, filename);
        result = createSubResult(FAIL, feedback);
        return result;
    }
}

SubTestRec outputTest2(int testNum, int subTest) {
    SubTestRec result;
    char feedback[300];
    char filename[] = "studentOutput/foldedCalendar2.ics";
    Calendar *testCal = foldedCalendar2();
    Calendar *refCal;

    writeCalendar(filename, testCal);

    ICalErrorCode err = createCalendar(filename, &refCal);

    if (err != OK) {
        sprintf(feedback, "Subtest %d.%d: Did not return OK when parsing a valid file (%s %s).", testNum, subTest, printError(err), filename);
        result = createSubResult(FAIL, feedback);
        return result;
    }

    if (calEqual(testCal, refCal)) {
        sprintf(feedback, "Subtest %d.%d: file %s parsed correctly",testNum,subTest,filename);
        result = createSubResult(SUCCESS, feedback);
        return result;
    }else{
        sprintf(feedback, "Subtest %d.%d: did not correctly parse a valid file (%s)",testNum,subTest, filename);
        result = createSubResult(FAIL, feedback);
        return result;
    }

}

SubTestRec outputTest3(int testNum, int subTest) {
    SubTestRec result;
    char feedback[300];
    char filename[] = "studentOutput/foldedCalendar3.ics";
    Calendar *testCal = foldedCalendar3();
    Calendar *refCal;

    writeCalendar(filename, testCal);
    ICalErrorCode err = createCalendar(filename, &refCal);

    if (err != OK) {
        sprintf(feedback, "Subtest %d.%d: Did not return OK when parsing a valid file (%s).", testNum, subTest, filename);
        result = createSubResult(FAIL, feedback);
        return result;
    }

    if (calEqual(testCal, refCal)) {
        sprintf(feedback, "Subtest %d.%d: file %s parsed correctly",testNum,subTest,filename);
        result = createSubResult(SUCCESS, feedback);
        return result;
    }else{
        sprintf(feedback, "Subtest %d.%d: did not correctly parse a valid file (%s)",testNum,subTest, filename);
        result = createSubResult(FAIL, feedback);
        return result;
    }
}


testRec* outputTest(int testNum) {
    const int numSubs = 3;
    int subTest = 1;
    char feedback[300];

    sprintf(feedback, "Test %d: Testing Output of Calendar Objects using writeCalendar() (15%%)", testNum);
    testRec* rec = initRec(testNum, numSubs, feedback);


    runSubTest(testNum, subTest, rec, &outputTest1);
    subTest++;

    runSubTest(testNum, subTest, rec, &outputTest2);

    subTest++;
    runSubTest(testNum, subTest, rec, &outputTest3);



    return rec;
}

SubTestRec megaCalRead(int testNum, int subTest) {
    SubTestRec result;
    char feedback[300];
    char filename[] = "testFiles/validA2/megaCal1.ics";
    Calendar *testCal;                  //foldedCalendar3();
    Calendar *refCal = megaCal();

    ICalErrorCode err = createCalendar(filename, &testCal);

    if (err != OK) {
        sprintf(feedback, "Subtest %d.%d: Did not return OK when parsing a valid file (%s).", testNum, subTest, filename);
        result = createSubResult(FAIL, feedback);
        return result;
    }

    if (calEqual(testCal, refCal)) {
        sprintf(feedback, "Subtest %d.%d: file %s parsed correctly",testNum,subTest,filename);
        result = createSubResult(SUCCESS, feedback);
        return result;
    }else{
        sprintf(feedback, "Subtest %d.%d: did not correctly parse a valid file (%s)",testNum,subTest, filename);
        result = createSubResult(FAIL, feedback);
        return result;
    }
}

SubTestRec megaCalWrite(int testNum, int subTest) {
    SubTestRec result;
    char feedback[300];
    char filename[] = "studentOutput/megaCal.ics";
    Calendar *testCal = megaCal();
    Calendar *refCal;

    writeCalendar(filename, testCal);
    ICalErrorCode err = createCalendar(filename, &refCal);
    //printf("Error code: %d", err);
    if (err != OK) {
        sprintf(feedback, "Subtest %d.%d: Did not return OK when parsing a valid file (%s), returned (%d).", testNum, subTest, filename, err);
        result = createSubResult(FAIL, feedback);
        return result;
    }

    if (calEqual(testCal, refCal)) {
        sprintf(feedback, "Subtest %d.%d: file %s parsed correctly",testNum,subTest,filename);
        result = createSubResult(SUCCESS, feedback);
        return result;
    }else{
        sprintf(feedback, "Subtest %d.%d: did not correctly parse a valid file (%s)",testNum,subTest, filename);
        result = createSubResult(FAIL, feedback);
        return result;
    }
}

SubTestRec megaCalFolding(int testNum, int subTest) {
    SubTestRec result;
    char feedback[300];
    char filename[] = "testFiles/validA2/megaCalFolded1.ics";
    Calendar *testCal = megaLineFolding();
    Calendar *refCal;

    ICalErrorCode err = createCalendar(filename, &refCal);

    if (err != OK) {
        sprintf(feedback, "Subtest %d.%d: Did not return OK when parsing a valid file (%s), returned (%d).", testNum, subTest, filename, err);
        result = createSubResult(FAIL, feedback);
        return result;
    }

    if (calEqual(testCal, refCal)) {
        sprintf(feedback, "Subtest %d.%d: file %s parsed correctly",testNum,subTest,filename);
        result = createSubResult(SUCCESS, feedback);
        return result;
    }else{
        sprintf(feedback, "Subtest %d.%d: did not correctly parse a valid file (%s) returned(%d)",testNum,subTest, filename, err);
        result = createSubResult(FAIL, feedback);
        return result;
    }

    return result;
}

SubTestRec megaCalFoldingWrite(int testNum, int subTest) {
    SubTestRec result;
    char feedback[300];
    char filename[] = "studentOutput/megaCalFolded.ics";
    Calendar *testCal = megaLineFolding();
    Calendar *refCal = NULL;

    writeCalendar(filename, testCal);
    ICalErrorCode err = createCalendar(filename, &refCal);
    //printf("Error code: %d", err);
    if (err != OK) {
        sprintf(feedback, "Subtest %d.%d: Did not return OK when parsing a valid file (%s), returned (%d).", testNum, subTest, filename, err);
        result = createSubResult(FAIL, feedback);
        return result;
    }

    if (calEqual(testCal, refCal)) {
        sprintf(feedback, "Subtest %d.%d: file %s parsed correctly",testNum,subTest,filename);
        result = createSubResult(SUCCESS, feedback);
        return result;
    }else{
        sprintf(feedback, "Subtest %d.%d: did not correctly parse a valid file (%s)",testNum,subTest, filename);
        result = createSubResult(FAIL, feedback);
        return result;
    }

}

testRec* megaCalTest(int testNum) {
    const int numSubs = 3;
    int subTest = 1;
    char feedback[300];

    sprintf(feedback, "Test %d: Mega Calendar Test (5%%)", testNum);
    testRec* rec = initRec(testNum, numSubs, feedback);

    runSubTest(testNum, subTest, rec, &megaCalRead);
    subTest++;

    runSubTest(testNum, subTest, rec, &megaCalWrite);
    subTest++;

    runSubTest(testNum, subTest, rec, &megaCalFoldingWrite);


    return rec;

}
