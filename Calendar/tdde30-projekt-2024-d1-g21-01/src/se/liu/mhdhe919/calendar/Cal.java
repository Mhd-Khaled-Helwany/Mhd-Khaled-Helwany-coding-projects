package se.liu.mhdhe919.calendar;

import se.liu.mhdhe919.calendar.gui.CalListener;
import se.liu.mhdhe919.calendar.time_units.CalDate;
import se.liu.mhdhe919.calendar.gui.CalViewer;
import se.liu.mhdhe919.calendar.time_units.Hour;
import se.liu.mhdhe919.calendar.time_units.Minute;
import se.liu.mhdhe919.calendar.time_units.Month;
import se.liu.mhdhe919.calendar.time_units.TimePeriod;
import se.liu.mhdhe919.calendar.time_units.TimeSpan;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;
import java.time.LocalDate;
import java.util.logging.*;

/**
 * The class responsible for creating the calendar itself
 * Is a collection of Appointment datatypes to help the user stay organized
 * Uses also a String datatype to allow the user to give the calendar a custome name
 * Can be also considered the UI part of the code since it contains methods
 triggered by user input
 */

public class Cal
{
    /** Constants */
    final private static int CURRENT_YEAR = LocalDate.now().getYear(); /** The minimum selectable year for the calendar */
    final private static int MAX_YEAR = LocalDate.now().getYear() + 10; /** The maximum selectable year for the calendar (10 years from the current year, magic number is fine) */
    final private static int IGNORE = 1; /** Used to ignore irrelevant requirements */
    private static final Logger LOGGER = Logger.getLogger(Cal.class.getName());

    static {
	try {
	    // Use the logger directory for the log file
	    Handler fileHandler = new FileHandler("logger" + File.separator + "Cal.log", true);
	    fileHandler.setFormatter(new SimpleFormatter());
	    LOGGER.addHandler(fileHandler);
	    LOGGER.setLevel(Level.ALL);
	} catch (IOException e) { // User is warned with an error message, no fallthrough
	    LOGGER.log(Level.SEVERE, "Failed to initialize logger handler.", e);
	    CalViewer.displayErrorMessage("An error occured while trying to create logging files, " +
					  "please check your files and try again");
	}
    }
    /** Instance variables */
    private List<Appointment> appointments;
    private String name;
    private List<Appointment> removedAppointments = new ArrayList<>(); /** Store the removed appointments to be used in the undo feature */
    private List<CalListener> calListeners = new ArrayList<>(); /** Used to keep track of changes in teh calendar */
    private boolean enabled = false; /** user chooses if the notifications are turned on or not with off as default */


    public Cal(final String name) {
	appointments = new ArrayList<>();
	this.name = name;
    }

    /** Necessary getters and setters */
    public List<Appointment> getAppointments() {
	return appointments;
    }

    public String getName() {
	return name;
    }

    public boolean isEnabled() {
	return enabled;
    }

    public void setAppointments(final List<Appointment> appointments) {
	this.appointments = appointments;
    }

    public void setName(final String name) {
	this.name = name;
    }

    public void setEnabled(final boolean enabled) {
	this.enabled = enabled;
    }

    public void showDay(int day, int month, int year) {
	// Get the name of the month
	String monthName = Month.getMonthName(month);

	// Check for valid input
	if (requirements(year, monthName, day, IGNORE, IGNORE, IGNORE, IGNORE)) {
	    // Display error message if input is invalid
	    CalViewer.displayErrorMessage("Cannot show appointments for that date, please check your input and try again");
	    return;
	}
	List<Appointment> appointmentsFound = dayAppointments(day, monthName, year);

	// Display appointments for the specified day
	CalViewer.displayDay(appointmentsFound);
    }

    /** Retrieves a list of appointments for a specific date */
    public List<Appointment> dayAppointments(int day, String month, int year) {
	// Initialize a list to store appointments found for the specified day
	List<Appointment> appointmentsFound = new ArrayList<>();

	// Create a CalDate object for the specified date
	Month mon = new Month(month, Month.getMonthNumber(month));
	CalDate date = new CalDate(day, mon, year);

	// Iterate through all appointments
	for (Appointment appointment : appointments) {
	    // Check if the appointment matches the specified date
	    if (date.equals(appointment.getDate())) {
		// Add the appointment to the list of appointments found for the specified day
		appointmentsFound.add(appointment);
	    }
	}
	return appointmentsFound;
    }

    private void sort() {
	// Create a comparator to sort appointments based on various criteria
	Comparator<Appointment> appointmentComparator = Comparator // Not possible to avoid repeated expression warning as this is how the class is used
		.comparing((Appointment a) -> a.getDate().getYear())
		.thenComparing((Appointment a) -> a.getDate().getMonth().getNumber())
		.thenComparing((Appointment a) -> a.getDate().getDay())
		.thenComparing((Appointment a) -> a.getTimespan().getStart().getHourValue())
		.thenComparing((Appointment a) -> a.getTimespan().getStart().getMinuteValue());

	// Sort appointments using the created comparator
	appointments.sort(appointmentComparator);

	// save the changes with calFile
	CalFile calFile = new CalFile(this);
	try {
	    calFile.saveCal();
	}catch (IOException e){  // No catchfallthrough, user gets notified of the problem so that they can try to fix it and no crashes occur
	    LOGGER.log(Level.SEVERE, "Check if enough space is available or for other interferences", e);
	    CalViewer.displayErrorMessage("an error occured while trying to save changes");
	}
	// Notify listeners after sorting appointments
	notifyListeners();
    }

    private static boolean requirements(final int year, final String month, final int day, final int startHour, final int startMinute,
					final int endHour, final int endMinute) {
	// Check if input values meet the requirements
	return year < CURRENT_YEAR || year > MAX_YEAR ||
	       Month.getMonthNumber(month) == -1 ||
	       day < 1 || day > Month.getMonthDays(month) ||
	       !(0 <= startHour && startHour <= 23) || !(0 <= endHour && endHour <= 23) ||
	       !(0 <= startMinute && startMinute <= 59) || !(0 <= endMinute && endMinute <= 59);
    }
    public void book(int year, String month, int day,
		     int startHour, int startMinute, int endHour,
		     int endMinute, String subject) {
	// Check if input values meet the requirements
	if (requirements(year, month, day, startHour, startMinute, endHour, endMinute)) {
	    // Display error message if input is invalid and log the error
	    LOGGER.log(Level.WARNING, "Attempted to book an overlapping appointment");
	    CalViewer.displayErrorMessage("Invalid input, please try again");
	    return;
	}

	// Create Minute, Hour, Month, CalDate, TimePeriod, and TimeSpan objects
	Minute startMin = new Minute(startMinute);
	Minute endMin = new Minute(endMinute);
	Hour startH = new Hour(startHour);
	Hour endH = new Hour(endHour);
	Month mon = new Month(month, Month.getMonthNumber(month));
	CalDate date = new CalDate(day, mon, year);
	TimePeriod start = new TimePeriod(startH, startMin);
	TimePeriod end = new TimePeriod(endH, endMin);

	// Check if start and end time periods are identical
	if (start.equals(end)) {
	    CalViewer.displayErrorMessage("The inputed start and end time periods are identical, please try again");
	    return;
	} else if (end.periodPrecedes(start)) {
	    // Check if end time precedes start time
	    CalViewer.displayErrorMessage("The inputed end time precedes the inputed start, please try again");
	    return;
	}

	TimeSpan timeSpan = new TimeSpan(start, end);
	// Check for overlapping appointments
	boolean overlap = false;
	for (Appointment appointment : appointments) {
	    if (date.equals(appointment.getDate()) && timeSpan.overlap(appointment.getTimespan())) {
		overlap = true;
		break;
	    }
	}
	if (overlap) {
	    // Display error message if appointment overlaps with existing appointments
	    CalViewer.displayErrorMessage("The appointment you are trying to book overlaps with another appointment, " +
					  "please remove the overlapping appointment or change the booking time");
	    return;
	}

	// Create Appointment object and add it to the list of appointments
	Appointment appointment = new Appointment(subject, date, timeSpan);
	appointments.add(appointment);

	// Sort appointments and notify listeners
	sort();
	CalViewer.displaySuccessMessage("Appointment booked successfully!");
    }
    public void remove(int year, String month, int day, int startHour, int startMinute) {
	// Check if input values meet the requirements
	if (requirements(year, month, day, startHour, startMinute, IGNORE, IGNORE)) {
	    // Display error message if input is invalid and log it
	    LOGGER.log(Level.WARNING, "Attempted to remove a non-existing appointment");
	    CalViewer.displayErrorMessage("Invalid input, please try again");
	    return;
	}

	// Create Minute, Hour, TimePeriod, Month, and CalDate objects
	Minute startMin = new Minute(startMinute);
	Hour startH = new Hour(startHour);
	TimePeriod time = new TimePeriod(startH, startMin);
	Month mon = new Month(month, Month.getMonthNumber(month));
	CalDate date = new CalDate(day, mon, year);

	// Flag to check if an appointment is removed
	boolean removed = false;

	// Iterate through appointments to find and remove the specified appointment
	for (Appointment appointment : appointments) {
	    if (appointment.getTimespan().getStart().equals(time)
		&& appointment.getDate().equals(date)) {
		removed = true;
		// Remove the appointment from the list and add it to removed list
		appointments.remove(appointment);
		removedAppointments.add(appointment);
		// Sort appointments and notify listeners
		sort();
		CalViewer.displaySuccessMessage("Appointment removed successfully!");
		break;
	    }
	}

	// Display error message if appointment was not found
	if (!removed) {
	    CalViewer.displayErrorMessage("Could not find the appointment, please check the inputed time and try again");
	}
    }
    public void undo(){
	// Check if there are any removed appointments
	if (removedAppointments.isEmpty()) {
	    CalViewer.displayErrorMessage("No appointments have been removed so far!");
	    return;
	}
	// Get the index of the last appointment in the removed list
	int lastIndex = removedAppointments.size() - 1;

	// Get the latest removed appointment and add it to the calendar
	Appointment undoAppointment = removedAppointments.get(lastIndex);
	removedAppointments.remove(lastIndex);
	appointments.add(undoAppointment);

	sort(); // Sort the appointments
    }

    public void addCalListener(CalListener cl) {
	// Add a calendar listener
	calListeners.add(cl);
    }

    private void notifyListeners() {
	// Notify all calendar listeners about the change in calendar
	for (CalListener calListener : calListeners) {
	    calListener.calChanged();
	}
    }

}
