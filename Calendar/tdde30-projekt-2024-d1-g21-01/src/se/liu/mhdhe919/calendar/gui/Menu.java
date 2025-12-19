package se.liu.mhdhe919.calendar.gui;

import se.liu.mhdhe919.calendar.Cal;

import javax.swing.*;

/**
 * The class responsible for creating the menu bar of the calendar to give the user easy access to important
 functionalities such as exiting the program and turning notifications on/off
 * Uses a JFrame, a Cal and a CalComponent to successfully overlay a menu bar
 */

public class Menu {

    private final JFrame frame;
    private final Cal calendar;
    private final CalComponent component;

    public Menu(JFrame frame, Cal calendar, CalComponent component) {
	this.frame = frame;
	this.calendar = calendar;
	this.component = component;
    }
    public void start(){
	// create the menu bar, menus and menu items
	JMenuBar menuBar = new JMenuBar();
	JMenu fileMenu = new JMenu("File");
	JMenu editMenu = new JMenu("Edit");
	JMenuItem exit = new JMenuItem("Exit");
	JMenuItem notifications = new JMenuItem("notifications");
	JMenuItem undo = new JMenuItem("Undo");
	fileMenu.add(exit);
	fileMenu.add(notifications);
	editMenu.add(undo);
	// ActionListeners for the menu items
	exit.addActionListener(e -> {
	    int choice = JOptionPane.showConfirmDialog(frame, "Are you sure you want to exit the application?", "Exit", JOptionPane.YES_NO_OPTION);
	    if (choice == JOptionPane.YES_OPTION) {
		System.exit(0);
	    }
	});
	notifications.addActionListener(e -> {
	    calendar.setEnabled(!calendar.isEnabled()); // toggle the setting
	    component.calChanged();
	});
	undo.addActionListener(e -> {
	    calendar.undo(); // perform undo action
	    component.calChanged();
	});
	// add the menus and finally the menu bar
	menuBar.add(fileMenu);
	menuBar.add(editMenu);
	frame.setJMenuBar(menuBar);
    }
}
