# /*
#  * CIS2750 F2017
#  * Assignment 3
#  * Jackson Zavarella 0929350
#  * This file creates the GUI and handles most of the business logic
#  * No code was used from previous classes/ sources
#  */

import tkinter as tk
from tkinter import ttk
from tkinter import *
import sys
from tkinter import messagebox, filedialog
from time import gmtime, strftime

from cal import driver


class App(tk.Tk):

    def __init__(self, title):
        tk.Tk.__init__(self)
        self.title(title) # Set the title of the application
        self.protocol('WM_DELETE_WINDOW', self.quitHandler)  # Ask the user if they really want to quit when they hit the x button
        # self.geometry("740x1200") # Set the dimension of the window
        width, height = self.winfo_screenwidth(), self.winfo_screenheight()
        self.geometry('%dx%d+0+0' % (width,height))
        self.mainFrame = tk.Frame(self)
        self.initMenuBar() # Make the menubar
        self.initFileView() # Make the file view
        self.initLogPanel() # Make the console
        self.mainFrame.pack(fill="both", expand=1)

        self.modalActive = False
        self.calPointer = None
        self.f = None

        self.calDriver = driver.CalendarDriver("bin/libcparse.so") # Initialize the CalendarDriver

    def initFileView(self):
        fileViewFrame = tk.Frame(self.mainFrame, bd=3, relief=SUNKEN)
        paddedFrame = tk.Frame(fileViewFrame) # Set a padding frame so we can pad the scroll bar and the treeview at the same time
        self.components = ttk.Treeview(paddedFrame, columns=("event", "props", "alarms", "sum"), height=25, selectmode="none") # Create the treeview select mode is none so we ca have out own implementation
        self.components['show'] = 'headings'
        ## Set the size of each column
        self.components.column("event", width=50, anchor=CENTER)
        self.components.column("props", width=50, anchor=CENTER)
        self.components.column("alarms", width=50, anchor=CENTER)
        self.components.column("sum", stretch=True) # Allow the summary to fill all remaining space

        ## Set the headers
        self.components.heading("event", text="Event No.")
        self.components.heading("props", text="Props")
        self.components.heading("alarms", text="Alarms")
        self.components.heading("sum", text="Summary")

        scrollbar = tk.Scrollbar(paddedFrame, command=self.components.yview) # Make a scrollbar and marry it to the frame
        scrollbar.pack(side="right", fill="y") # Pin the scrollbar to the right side and take up all room
        self.components.config(yscrollcommand=scrollbar.set) # Tell the text that it loves the scrollbar even though she is married ot the padding frame
        paddedFrame.pack(fill="both", padx=10, pady=10)
        fileViewFrame.pack(fill="both")
        self.components.pack(fill="both")
        self.components.bind("<ButtonPress-1>", lambda event: self.rowClickHandler(event, self.components)) # Handle clicking of a row

    def initLogPanel(self):
        logFrame = tk.Frame(self.mainFrame) # Create a frame to house the console
        paddedFrame = tk.Frame(logFrame, bd=3, relief=SUNKEN) # Create a frame to put both the console and scrollbar in which allows padding
        buttonFrame = tk.Frame(logFrame)
        buttonFrame.pack(fill="x", padx=10)
        clearButton = Button(buttonFrame, text="Clear Log", command=lambda: self.clearLog())
        clearButton.pack(side=RIGHT)
        self.console = tk.Text(paddedFrame, wrap="word", state=DISABLED) # Disable editing of the text
        scrollbar = tk.Scrollbar(paddedFrame, command=self.console.yview) # Make a scrollbar and marry it to the frame
        scrollbar.pack(side="right", fill="y") # Pin the scrollbar to the right side and take up all room
        self.console.config(yscrollcommand=scrollbar.set) # Tell the text that it loves the scrollbar even though she is married ot the padding frame

        logFrame.pack(fill="both")
        paddedFrame.pack(fill="both", padx=10, pady=10)
        self.console.pack(side="left", fill="both", expand=True)
        self.console.bind("<1>", lambda event: self.console.focus_set()) # Allow the user to only copy text from the console

    def initMenuBar(self):
        menubar = tk.Menu(self)
        fileMenu = tk.Menu(menubar, tearoff=False)

        fileMenu.add_command(label="Open", command=self.openHandler, accelerator="Ctrl+o")
        fileMenu.add_command(label="Save", command=self.saveHandler, accelerator="Ctrl+s")
        fileMenu.add_command(label="Save as..", command=self.saveAsHandler, accelerator="Shift+s")
        fileMenu.add_command(label="Exit", underline=1, command=self.quitHandler, accelerator="Ctrl+x")

        self.createMenu = tk.Menu(menubar, tearoff=False)
        self.createMenu.add_command(label="Create calendar", command=self.createCalendarHandler, accelerator="Ctrl+c")
        self.createMenu.add_command(label="Create event", command=lambda: self.createEventHandler(self), accelerator="Ctrl+e", state="disabled")

        def handleEventSuccess(uid, date, start):
            self.calDriver.addEvent(self.calPointer, uid, date, start)
            self.log("Successfully added a new event")
            self.updateComponentView()

        self.handleEventSuccess = handleEventSuccess

        self.showMenu = tk.Menu(menubar, tearoff=False)
        self.showMenu.add_command(label="Show alarms", command=self.showAlarmsHandler, accelerator="Ctrl+a", state="disabled")
        self.showMenu.add_command(label="Show external props", command=self.showPropsHandler, accelerator="Ctrl+p", state="disabled")

        helpMenu = tk.Menu(menubar, tearoff=False)
        helpMenu.add_command(label="About iCalGUI", command=self.aboutHandler, accelerator="Ctrl+i")

        menubar.add_cascade(label="File", underline=0, menu=fileMenu)
        menubar.add_cascade(label="Create", underline=0, menu=self.createMenu)
        menubar.add_cascade(label="Show", underline=0, menu=self.showMenu)
        menubar.add_cascade(label="Help", underline=0, menu=helpMenu)
        self.config(menu=menubar)

        self.bind_all("<Control-o>", lambda event: self.after(150, self.openHandler)) # OSX bug requires this to be present
        self.bind_all("<Control-s>", lambda event: self.after(150, self.saveHandler))
        self.bind_all("<S>", lambda event: self.after(150, self.saveAsHandler))
        self.bind_all("<Control-x>", lambda event: self.after(150, self.quitHandler))
        self.bind_all("<Control-c>", lambda event: self.after(150, self.createCalendarHandler))
        self.bind_all("<Control-e>", lambda event: self.after(150, self.createEventHandler(self)))
        self.bind_all("<Control-a>", lambda event: self.after(150, self.showAlarmsHandler))
        self.bind_all("<Control-p>", lambda event: self.after(150, self.showPropsHandler))
        self.bind_all("<Control-i>", lambda event: self.after(150, self.aboutHandler))

    def updateComponentView(self):
        self.components.delete(*self.components.get_children()) # Remove old components
        components = self.calDriver.getCalendarComponents(self.calPointer)
        for component in components:
            self.insertComponent(component)

    def clearLog(self):
        self.console.config(state=NORMAL) # Allow editting
        self.console.delete(1.0, END)
        self.console.config(state=DISABLED) # Disable editting

    def insertComponent(self, component):
        n = len(self.components.get_children("")) + 1 # Get number of elements in list and add 1
        summary = component[2]
        self.components.insert("" , "end", values=(n, component[0], component[1], summary)) # Put the component in the list

    def log(self, log):
        self.console.config(state=NORMAL) # Allow editting
        self.console.insert(END, log) # Push the log into the end of the console
        self.console.insert(END, "\n") # Push the log into the end of the console
        self.console.config(state=DISABLED) # Disable editting
        self.console.see(END) # Ensure the console is scrolled to the end
        print(log) # Log the log in the console as well

    def rowClickHandler(self, event, treeview):
        row = treeview.selection() # Get all selected nodes
        clicked = treeview.identify_row(event.y) # Get the node our mouse is on
        if clicked == '':
            return
        if clicked not in row: # If the node our mouse is on is not selected
            treeview.selection_add(clicked) # Select it
            self.enableShowMenu()
        else:
            self.disableShowMenu()
        treeview.selection_remove(row) # Deselect all other nodes

    def openHandler(self):
        if self.modalActive == True:
            self.log("Please finish what you are doing before attempting to open a file.")
            return
        self.f = filedialog.askopenfilename(initialdir = ".",title = "Select file",filetypes = (("all files","*.*"), ("iCalendar files","*.ics")))
        if self.f == '':
            self.f == None
            return

        if self.calPointer != None:
            answer = messagebox.askquestion("Warning", "You potentially have unsaved work, opening this calendar will overwrite any unsaved changes you currently have open.\nDo you wish to continue?", icon='warning')
            if answer == "no":
                return
        self.fPretty = self.f[self.f.rfind("/") + 1:]
        self.log("Opening : " + self.fPretty) # Tell the user we are trying to open the file
        calPointer = self.calDriver.openCalendar(self.f) # Try to open the file
        if calPointer == None:
            error = self.calDriver.validateCalendar(self.f)
            self.log("Error opening calendar file " + self.fPretty + " : " + error) # Output the error
            return
        if self.calPointer != None:
            self.calDriver.deleteCalendar(self.calPointer)

        self.log("Successfully opened the file " + self.fPretty) # Tell the user everything went OK
        self.title(self.f)

        self.calPointer = calPointer
        self.createMenu.entryconfig("Create event", state="normal")
        self.updateComponentView()

    def saveHandler(self):
        if self.modalActive == True:
            self.log("Please finish what you are doing before attempting to save a file.")
            return
        if len(self.components.get_children()) == 0:
            messagebox.showerror("Error", "You must add some events before saving. (ctrl+e)")
            self.log("You must add some events before saving. (ctrl+e)")
            return
        if self.calPointer == None:
            self.log("Nothing to save.")
            return
        if self.f != None: # If a file has been opened / saved
            response = self.calDriver.writeCalendar(self.f, self.calPointer)
            if response != "OK":
                self.log("Failed to save file " + self.f + ".\nReceived error: " + response)
                return
            self.log("Successfully saved file " + self.f)
            self.title(self.f)
            return
        self.saveAsHandler()

    def saveAsHandler(self):
        if self.modalActive == True:
            self.log("Please finish what you are doing before attempting to save a file.")
            return
        if len(self.components.get_children()) == 0:
            messagebox.showerror("Error", "You must add some events before saving. (ctrl+e)")
            self.log("You must add some events before saving. (ctrl+e)")
            return
        if self.calPointer == None:
            self.log("Nothing to save.")
            return
        f = filedialog.asksaveasfile(mode='w', defaultextension=".ics")
        if f == None:
            return
        self.f = f.name
        self.saveHandler()

    def quitHandler(self):
        result = messagebox.askquestion("Leaving so soon?", "Are you sure you want to quit? :(", icon='warning') # Prompt the user if they wish to exit
        if result == 'no':
            return
        self.log("quitting...")
        if self.calPointer != None:
            self.calDriver.deleteCalendar(self.calPointer)
        sys.exit(0)

    def createCalendarHandler(self):
        if self.modalActive == True:
            return
        self.modalActive = True
        self.calModal = tk.Toplevel(master=self)
        self.calModal.modalActive = False
        self.calModal.transient(self) # Only show one window in the task bar
        self.calModal.grab_set()
        self.calModal.title("Create Calendar")
        paddedFrame = tk.Frame(self.calModal) # Create a padding frame frame
        self.calModal.protocol('WM_DELETE_WINDOW', lambda: self.cancelHandler(self.calModal, self))

        prodFrame = tk.Frame(paddedFrame)
        prodLabel = tk.Label(prodFrame, text="Prod ID:")
        prodLabel.pack(side=LEFT)
        prodId = tk.Entry(prodFrame, bd=2)
        prodId.pack(side=RIGHT)
        prodFrame.pack()

        eventFrame = tk.Frame(paddedFrame)
        eventLabel = tk.Label(eventFrame, text="Calendar Events:")
        eventLabel.pack(side=LEFT)
        eventButton = Button(eventFrame, text="Add Event", command=lambda: self.createEventHandler(self.calModal))
        eventButton.pack(side=RIGHT)
        eventFrame.pack()

        eventFrame = tk.Frame(paddedFrame)
        events = ttk.Treeview(eventFrame, columns=("uid", "create", "start"), height=4, selectmode="none") # Create the treeview select mode is none so we ca have out own implementation
        events['show'] = 'headings'
        ## Set the size of each column
        events.column("uid", minwidth=30, stretch=TRUE, anchor=CENTER)
        events.column("create", minwidth=30, stretch=TRUE, anchor=CENTER)
        events.column("start", minwidth=30, stretch=TRUE, anchor=CENTER)
        ## Set the headers
        events.heading("uid", text="UID")
        events.heading("create", text="Creation Date")
        events.heading("start", text="Start Date")
        eventscrollbar = tk.Scrollbar(eventFrame, command=events.yview) # Make a scrollbar and marry it to the frame
        eventscrollbar.pack(side="right", fill="y") # Pin the scrollbar to the right side and take up all room
        events.config(yscrollcommand=eventscrollbar.set) # Tell the text that it loves the scrollbar even though she is married ot the padding frame
        events.pack(fill="x")
        eventFrame.pack()

        buttonFrame = tk.Frame(paddedFrame)
        cancelButton = Button(buttonFrame, text="Cancel", command=lambda: self.cancelHandler(self.calModal, self))
        cancelButton.pack(side=LEFT)
        submitButton = Button(buttonFrame, text="Submit", command=lambda: self.submitCalendarHandler("2", prodId.get(), events, self.calModal))
        submitButton.pack(side=RIGHT)
        buttonFrame.pack(side=RIGHT)

        def handleEventSuccess(uid, date, start):
            events.insert("" , "end", values=(uid, date, start)) # Put the component in the list

        self.calModal.handleEventSuccess = handleEventSuccess

        paddedFrame.pack(padx=10, pady=10)
        self.wait_window(self.calModal)

    def destroyModal(self, modal):
        modal.grab_release()
        modal.destroy()

    def cancelHandler(self, modal, parent):
        self.destroyModal(modal)
        parent.modalActive = False

    def submitCalendarHandler(self, version, prodId, eventsTreeview, parent):
        if self.calDriver.validateVersion(version) == False:
            messagebox.showerror("Error", "You must enter a valid version.")
            return
        # if self.calDriver.matchTEXTField(prodId) == False:
        if prodId == "":
            messagebox.showerror("Error", "You must enter a valid product id.")
            return
        event = eventsTreeview.get_children()
        if len(event) == 0:
            answer = messagebox.askquestion("Warning", "You have not entered any events.\nYou will not be able to save before doing so.\nDo you wish to continue?", icon='warning')
            if answer == "no":
                return
        self.newCalendar(version, prodId, eventsTreeview)
        self.destroyModal(parent)
        self.modalActive = False

    def newCalendar(self, version, prodId, eventsTreeview):
        if self.calPointer != None:
            answer = messagebox.askquestion("Warning", "You potentially have unsaved work, submitting this calendar will overwrite any unsaved changes you currently have open.\nDo you wish to continue?", icon='warning')
            if answer == "no":
                return
            self.calDriver.deleteCalendar(self.calPointer)

        self.calPointer = self.calDriver.createBasicCalendar(float(version), prodId)
        self.createMenu.entryconfig("Create event", state="normal")

        for event in eventsTreeview.get_children():
            uid = eventsTreeview.item(event)['values'][0]
            create = eventsTreeview.item(event)['values'][1]
            start = eventsTreeview.item(event)['values'][2]
            self.calDriver.addEvent(self.calPointer, uid, create, start)

        self.log("Successfully created a new calendar")
        self.updateComponentView()

    def createEventHandler(self, parent):
        if self.createMenu.entrycget("Create event", "state") == "disabled" and self.modalActive == False:
            self.log("You must open or create an iCalendar file to be able to create an event")
            return
        if parent.modalActive == True:
            return
        parent.modalActive = True
        self.eventModal = tk.Toplevel(master=self)
        self.eventModal.transient(self) # Only show one window in the task bar
        self.eventModal.grab_set()
        self.eventModal.title("Create Event")
        paddedFrame = tk.Frame(self.eventModal) # Create a padding frame frame
        self.eventModal.protocol('WM_DELETE_WINDOW', lambda: self.cancelHandler(self.eventModal, parent))

        uidFrame = tk.Frame(paddedFrame)
        uidLabel = tk.Label(uidFrame, text="UID: ")
        uidLabel.pack(side=LEFT)
        uid = tk.Entry(uidFrame, bd=2)
        uid.pack(side=RIGHT)
        uidFrame.pack()

        dateFrame = tk.Frame(paddedFrame)
        dateLabel = tk.Label(dateFrame, text="Creation Date Time :")
        dateLabel.pack(side=LEFT)
        date = tk.Entry(dateFrame, bd=2)
        date.insert(0, strftime("%Y%m%dT%H%M%S", gmtime())) # Pre populate the time
        date.pack(side=RIGHT)
        dateFrame.pack()

        startFrame = tk.Frame(paddedFrame)
        startLabel = tk.Label(startFrame, text="Start Time(optional) :")
        startLabel.pack(side=LEFT)
        start = tk.Entry(startFrame, bd=2)
        start.pack(side=RIGHT)
        startFrame.pack()

        buttonFrame = tk.Frame(paddedFrame)
        cancelButton = Button(buttonFrame, text="Cancel", command=lambda: self.cancelHandler(self.eventModal, parent))
        cancelButton.pack(side=LEFT)
        submitButton = Button(buttonFrame, text="Submit", command=lambda: self.submitEventHandler(uid.get(), date.get(), start.get(), parent, self.eventModal))
        submitButton.pack(side=RIGHT)
        buttonFrame.pack(side=RIGHT)

        paddedFrame.pack(padx=10, pady=10)
        parent.wait_window(self.eventModal)

    def submitEventHandler(self, uid, date, start, parent, eventModal):
        if uid == "":
            messagebox.showerror("Error", "You must enter a valid UID.")
            return
        if self.calDriver.matchDATEField(date) == False:
            messagebox.showerror("Error", "You must enter a valid Creation Date Time.")
            return
        if start == "":
            start = date
        elif self.calDriver.matchDATEField(start) == False:
            messagebox.showerror("Error", "You must enter a valid Start Date.")
            return
        parent.handleEventSuccess(uid, date, start)
        self.destroyModal(eventModal)
        parent.modalActive = False

    def enableShowMenu(self):
        self.showMenu.entryconfig("Show alarms", state="normal")
        self.showMenu.entryconfig("Show external props", state="normal")

    def disableShowMenu(self):
        self.showMenu.entryconfig("Show alarms", state="disabled")
        self.showMenu.entryconfig("Show external props", state="disabled")

    def showAlarmsHandler(self):
        if self.showMenu.entrycget("Show alarms", "state") == "disabled":
            self.log("You must open an iCalendar file and select a component in order to be able to see alarms")
            return
        selected = self.components.selection()
        if self.components.item(selected)['values'][2] <= 0:
            self.log("Event number " + str(self.components.item(selected)['values'][0]) + " does not have any alarms.")
            return
        response = self.calDriver.getComponentAlarms(self.calPointer, self.components.item(selected)['values'][0])
        if response is None:
            self.log("Event number " + str(self.components.item(selected)['values'][0]) + " does not exist in " + self.fPretty + " or the file is no longer valid.\nThis could be caused by the iCalendar file being edited after it was opened here.")
            return
        self.log(response)

    def showPropsHandler(self):
        if self.showMenu.entrycget("Show external props", "state") == "disabled":
            self.log("You must open an iCalendar file and select a component in order to be able to see external properties")
            return;
        selected = self.components.selection()
        if self.components.item(selected)['values'][1] <= 3:
            self.log("Event number " + str(self.components.item(selected)['values'][0]) + " does not have any optional props.")
            return
        response = self.calDriver.getComponentProperties(self.calPointer, self.components.item(selected)['values'][0])
        if response is None:
            self.log("Event number " + str(self.components.item(selected)['values'][0]) + " does not exist in " + self.fPretty + " or the file is no longer valid.\nThis could be caused by the iCalendar file being edited after it was opened here.")
            return
        self.log(response);

    def aboutHandler(self):
        messagebox.showinfo("About", "iCalendar GUI for CIS*2750\n\nUsed to display and edit iCalendar files\n\nCreated by yours truly, \nJackson Zavarella", icon='info')
