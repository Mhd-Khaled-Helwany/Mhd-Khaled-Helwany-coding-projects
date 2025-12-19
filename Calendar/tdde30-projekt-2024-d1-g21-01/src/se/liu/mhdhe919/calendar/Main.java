package se.liu.mhdhe919.calendar;

import se.liu.mhdhe919.calendar.gui.CalComponent;
import se.liu.mhdhe919.calendar.gui.CalViewer;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.util.logging.*;

/**
 * The class that runs the calendar program for the user,
   combines all parts of the project to achieve that
 */

public class Main
{
    private static final Logger LOGGER = Logger.getLogger(Main.class.getName());

    static {
	try {
	    // Use the logger directory for the log file
	    Handler fileHandler = new FileHandler("logger" + File.separator + "Main.log", true);
	    fileHandler.setFormatter(new SimpleFormatter());
	    LOGGER.addHandler(fileHandler);
	    LOGGER.setLevel(Level.ALL);
	} catch (IOException e) { // User is warned with an error message, no fallthrough
	    LOGGER.log(Level.SEVERE, "Failed to initialize logger handler.", e);
	    CalViewer.displayErrorMessage("An error occured while trying to create logging files, " +
					  "please check your files and try again");
	}
    }
    public static void main(String[] args){
	Cal calendar = new Cal(null);
	CalFile calFile = new CalFile(calendar);
	CalComponent component= new CalComponent(calendar);
	calendar.addCalListener(component);
	CalViewer calViewer = new CalViewer(calendar);
	Notification notification = new Notification(calendar,calViewer);
	boolean fileReadSuccessful = false;
	while (!fileReadSuccessful) {   // Allow the user to keep trying solutions until it works
	    try {
		calFile.readCal();
		fileReadSuccessful = true;
	    } catch (FileNotFoundException e) { // While loop allows the user to keep trying, no fallthrough
		calViewer.displayFileError(calFile, "Could not find an existing calendar. Please try again or create a new one.", "FileNotFound");
		LOGGER.log(Level.SEVERE, "Retry loading the file or check if it is placed correctly", e);
	    } catch (IOException d) { // While loop allows the user to keep trying, no fallthrough
		calViewer.displayFileError(calFile, "An error occured while trying to read calendar file. Please try again.", "IO");
		LOGGER.log(Level.SEVERE, "Check if enough space is available or for other interferences", d);
	    }
	}
	notification.startNotificationTimer();
	calViewer.show();
    }
}
