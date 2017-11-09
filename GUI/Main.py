# !/usr/bin/python3
from tkinter import *
from frame import calGUI

def main():
    # root = Tk()
    # root.geometry("250x150+300+300")
    # app = frame.MainFrame("Assignment 3")
    # root.mainloop()
    app = calGUI.App("iCalGUI")
    app.mainloop()

if __name__ == '__main__':
    main()
