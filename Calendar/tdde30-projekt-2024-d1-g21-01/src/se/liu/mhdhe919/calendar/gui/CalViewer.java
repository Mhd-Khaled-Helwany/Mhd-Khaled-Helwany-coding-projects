package se.liu.mhdhe919.calendar.gui;

import se.liu.mhdhe919.calendar.Appointment;
import se.liu.mhdhe919.calendar.Cal;
import se.liu.mhdhe919.calendar.CalFile;

import javax.swing.*;
import java.awt.*;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.util.List;
import java.util.Objects;
import java.util.logging.*;

/**
 * The class responsible for displaying the calendar and any popups that the program produces
 * Uses a Cal instance to display the unique calendar
 */

public class CalViewer {
     private Cal calendar;
     private final static int MAX_NAME_CHAR = 20;
    private static final Logger LOGGER = Logger.getLogger(CalViewer.class.getName());

    static {
	try {

	    // Use the logger directory for the log file
	    Handler fileHandler = new FileHandler("logger" + File.separator + "CalViewer.log", true);
	    fileHandler.setFormatter(new SimpleFormatter());
	    LOGGER.addHandler(fileHandler);
	    LOGGER.setLevel(Level.ALL);
	} catch (IOException e) { // User is warned with an error message, no fallthrough
	    LOGGER.log(Level.SEVERE, "Failed to initialize logger handler.", e);
	    displayErrorMessage("An error occured while trying to create logging files, " +
					  "please check your files and try again");
	}
    }

    public CalViewer(final Cal calendar) {
	this.calendar = calendar;
    }

    /** Displays the calendar in a graphical user interface */
    public void show() {
	JFrame frame = new JFrame("Calendar");
	CalComponent component = new CalComponent(calendar);
	Menu menu = new Menu(frame, calendar, component);
	menu.start();
	frame.add(component, BorderLayout.CENTER);
	Start startScreen = new Start(component.getPreferredSize());
	frame.add(startScreen, BorderLayout.CENTER);
	frame.pack();
	frame.setVisible(true);
	frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);

	// Prompt the user to name their calendar if no saved calendar is found
	if (Objects.equals(calendar.getName(), "null")){
	    displayNamePopup(frame);
	}

	// Display the welcome screen for 3 seconds
	Timer timer = new Timer(3000, e -> {
	    frame.remove(startScreen);
	    frame.add(component, BorderLayout.CENTER);
	    frame.revalidate();
	});
	timer.setRepeats(false);
	timer.start();
    }
    /** displays the input dialog for naming the calendar */
    private void displayNamePopup(JFrame frame){
	String calendarName = null;
	while (calendarName == null || calendarName.isEmpty() || calendarName.length() > MAX_NAME_CHAR) { // Necessary repetition for name requirements
	    calendarName = JOptionPane.showInputDialog(frame, "Please enter a name for your calendar (up to 20 characters):");
	    if (calendarName != null && calendarName.length() > MAX_NAME_CHAR) {
		CalViewer.displayErrorMessage("Calendar name must be 20 characters or less, please try again");
	    }
	}
	// Set the name for the calendar
	calendar.setName(calendarName);
    }
    /** Displays the appointments of a specific day in a dialog window */
    public static void displayDay(List<Appointment> appointments) {
	if (appointments.isEmpty()) {
	    // If no appointments found, display a message dialog
	    JOptionPane.showMessageDialog(null, "No appointments found for the day", "Appointments", JOptionPane.INFORMATION_MESSAGE);
	} else {
	    // If appointments found, prepare a message to display all appointments
	    StringBuilder appointmentMessage = new StringBuilder("Appointments for the day:\n");
	    for (Appointment appointment : appointments) {
		appointmentMessage.append(appointment).append("\n");
	    }
	    // Display a message dialog with all appointments
	    JOptionPane.showMessageDialog(null, appointmentMessage.toString(), "Appointments", JOptionPane.INFORMATION_MESSAGE);
	}
    }
    public void displayNotification(String appSubject){
	JOptionPane.showMessageDialog(null,appSubject + " is in 1 hour!","Notification", JOptionPane.INFORMATION_MESSAGE);
    }

    public static void displayErrorMessage(String message){
	// Display an error message dialog to the user
	JOptionPane.showMessageDialog(null, message, "Error", JOptionPane.ERROR_MESSAGE);
    }
    public static void displaySuccessMessage(String message){
	JOptionPane.showMessageDialog(null, message, "Success", JOptionPane.INFORMATION_MESSAGE);
    }

    /** Handles any errors that might occur with the calendar file from the Main class */
    public void displayFileError(CalFile calFile, String errorMessage, String errorType){
	// Handle the error depending on the errorType
	if (Objects.equals(errorType, "FileNotFound")) {
	    JDialog dialog = new JDialog();
	    dialog.setTitle("Error");
	    dialog.setSize(600, 150);
	    dialog.setLayout(new BorderLayout());
	    dialog.setModalityType(Dialog.ModalityType.APPLICATION_MODAL);

	    JLabel messageLabel = new JLabel(errorMessage);
	    dialog.add(messageLabel, BorderLayout.CENTER);

	    JPanel buttonPanel = new JPanel(new FlowLayout(FlowLayout.CENTER));
	    JButton tryAgainButton = new JButton("Try Again");
	    tryAgainButton.addActionListener(new ActionListener() {
		@Override
		public void actionPerformed(ActionEvent e) {
		    dialog.dispose();
		    try {
			calFile.readCal();
		    } catch (FileNotFoundException g) { // Main has a while loop that lets the user keep trying again, no fallthrough
			LOGGER.log(Level.SEVERE, "Retry loading the file or check if it is placed correctly", g);
		    } catch (IOException f) { // Main has a while loop that lets the user keep trying again, no fallthrough
			LOGGER.log(Level.SEVERE, "Check if enough space is available or for other interferences", f);
		    }
		}
	    });
	    buttonPanel.add(tryAgainButton);

	    JButton createNewButton = new JButton("Create New Calendar");
	    createNewButton.addActionListener(new ActionListener() {
		@Override
		public void actionPerformed(ActionEvent e) {
		    dialog.dispose();
		    try {
			calFile.saveCal();
		    } catch (IOException d) { // User gets an error message and the program does not crash, no fallthrough
			LOGGER.log(Level.SEVERE, "Check if enough space is available or for other interferences", d);
			displayErrorMessage("An error occured while trying to create a new calendar, please check if you have enough space and try again");
		    }
		}
	    });
	    buttonPanel.add(createNewButton);

	    dialog.add(buttonPanel, BorderLayout.SOUTH);
	    dialog.setVisible(true);
	} else {
	    boolean success = false;
	    while (!success) {  // Allow the user to keep trying solutions until it works
		try {
		    calFile.saveCal();
		    success = true;
		} catch (IOException s) { // User gets an error message and the program does not crash, no fallthrough
		    LOGGER.log(Level.SEVERE, "Check if enough space is available or for other interferences", s);
		    displayErrorMessage("An error occurred while trying to load the saved calendar. Please try again.");
		}
	    }
	}
    }
}
