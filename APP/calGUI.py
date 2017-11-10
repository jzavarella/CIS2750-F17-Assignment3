# !/usr/bin/python3

# TODO: Clean this up
import tkinter as tk
from tkinter import ttk
from tkinter import *
import sys
from tkinter import messagebox, filedialog

from cal import driver


class App(tk.Tk):

    def __init__(self, title):
        tk.Tk.__init__(self)
        self.title(title) # Set the title of the application
        self.protocol('WM_DELETE_WINDOW', self.quitHandler)  # Ask the user if they really want to quit when they hit the x button
        self.geometry("740x1200") # Set the dimension of the window
        self.initMenuBar() # Make the menubar
        self.initFileView() # Make the file view
        self.initLogPanel() # Make the console

        self.calDriver = driver.CalendarDriver("bin/libcparse.so") # Initialize the CalendarDriver


    def initFileView(self):
        fileViewFrame = tk.Frame(self, bg="RED")
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
        self.components.bind("<ButtonPress-1>", self.rowClickHandler) # Handle clicking of a row

    def initLogPanel(self):
        logFrame = tk.Frame(self, bg="BLUE") # Create a frame to house the console
        paddedFrame = tk.Frame(logFrame) # Create a frame to put both the console and scrollbar in which allows padding
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
        fileMenu.add_command(label="Save as..", command=self.saveAsHandler)
        fileMenu.add_command(label="Exit", underline=1, command=self.quitHandler, accelerator="Ctrl+x")

        createMenu = tk.Menu(menubar, tearoff=False)
        createMenu.add_command(label="Create calendar", command=self.createCalendarHandler, accelerator="Ctrl+c")
        createMenu.add_command(label="Create event", command=self.createEventHandler, accelerator="Ctrl+e")

        helpMenu = tk.Menu(menubar, tearoff=False)
        helpMenu.add_command(label="About iCalGUI", command=self.aboutHandler, accelerator="Ctrl+i")

        menubar.add_cascade(label="File", underline=0, menu=fileMenu)
        menubar.add_cascade(label="Create", underline=0, menu=createMenu)
        menubar.add_cascade(label="Help", underline=0, menu=helpMenu)
        self.config(menu=menubar)

        self.bind_all("<Control-o>", lambda event: self.after(150, self.openHandler)) # OSX bug requires this to be present
        self.bind_all("<Control-s>", lambda event: self.after(150, self.saveHandler))
        self.bind_all("<Control-x>", lambda event: self.after(150, self.quitHandler))
        self.bind_all("<Control-c>", lambda event: self.after(150, self.createCalendarHandler))
        self.bind_all("<Control-e>", lambda event: self.after(150, self.createEventHandler))
        self.bind_all("<Control-i>", lambda event: self.after(150, self.aboutHandler))

    def insertComponent(self, component):
        n = len(self.components.get_children("")) + 1 # Get number of elements in list and add 1
        self.components.insert("" , "end", values=(n, component[0], component[1], component[2])) # Put the component in the list

    def log(self, log):
        self.console.config(state=NORMAL) # Allow editting
        self.console.insert(END, log) # Push the log into the end of the console
        self.console.insert(END, "\n") # Push the log into the end of the console
        self.console.config(state=DISABLED) # Disable editting
        self.console.see(END) # Ensure the console is scrolled to the end
        print(log) # Log the log in the console as well

    def rowClickHandler(self, event):
        row = self.components.selection() # Get all selected nodes
        clicked = self.components.identify_row(event.y) # Get the node our mouse is on
        if clicked not in row: # If the node our mouse is on is not selected
            self.components.selection_add(clicked) # Select it
        self.components.selection_remove(row) # Deselect all other nodes

    def openHandler(self):
        f = filedialog.askopenfilename(initialdir = ".",title = "Select file",filetypes = (("all files","*.*"), ("iCalendar files","*.ics")))
        fPretty = f[f.rfind("/") + 1:]
        self.log("Opening : " + fPretty) # Tell the user we are trying to open the file
        valid = self.calDriver.validateCalendar(f) # Try to open the file
        if valid != "OK":
            self.log("Error opening calendar file " + fPretty + " : " + valid) # Output the error
            return
        self.components.delete(*self.components.get_children())
        self.log(valid) # Tell the user everything went OK
        self.title(fPretty)

        components = self.calDriver.getCalendarComponents(f)
        for component in components:
            self.insertComponent(component)

    def saveHandler(self):
        self.log("save")

    def saveAsHandler(self):
        self.log("save as")

    def quitHandler(self):
        result = messagebox.askquestion("Leaving so soon?", "Are you sure you want to quit? :(", icon='warning') # Prompt the user if they wish to exit
        if result == 'yes':
            self.log("quitting...")
            sys.exit(0)


    def createCalendarHandler(self):
        self.log("calendar")

    def createEventHandler(self):
        self.log("event")

    def aboutHandler(self):
        messagebox.showinfo("About", "iCalendar GUI for CIS*2750\n\nUsed to display and edit iCalendar files\n\nCreated by yours truly, \nJackson Zavarella", icon='info')
