package se.liu.mhdhe919.calendar;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.time.LocalDateTime;
import java.time.LocalTime;
import java.util.ArrayList;
import java.util.List;
import java.util.Timer;
import java.util.TimerTask;
import java.util.logging.*;
import javazoom.jl.decoder.JavaLayerException;
import javazoom.jl.player.Player;
import se.liu.mhdhe919.calendar.gui.CalViewer;
import se.liu.mhdhe919.calendar.time_units.Month;

/**
 * The class responsible for notifying users that an appointment is roughly one hour away
 * Uses a Cal and CalViewer instances to notify for the right calendar with a message window and a sound
 * Users can disable the notifications in the Menu bar from the File section
 */

public class Notification {
    private Cal calendar;
    private CalViewer calViewer;
    private final static int CHECK_RATE = 60000; /** checks every minute for a notification */
    private Timer timer = null; /** Declare the Timer object as a class-level field , explained in the report why*/
    private LocalDateTime currentDate = LocalDateTime.now();
    private String month = Month.getMonthName(currentDate.getMonthValue()); /** initialize the day we want to check outside the loop for efficiency */
    private List<Appointment> notifiedAppointments = new ArrayList<>(); /** to avoid notifying more than once for the same appointment */
    private final static int HOUR_IN_MINUTES = 60;
    private static final Logger LOGGER = Logger.getLogger(Notification.class.getName());

    static {
	try {
	    // Use the logger directory for the log file
	    Handler fileHandler = new FileHandler("logger" + File.separator + "Notification.log", true);
	    fileHandler.setFormatter(new SimpleFormatter());
	    LOGGER.addHandler(fileHandler);
	    LOGGER.setLevel(Level.ALL);
	} catch (IOException e) { // User is warned with an error message, no fallthrough
	    LOGGER.log(Level.SEVERE, "Failed to initialize logger handler.", e);
	    CalViewer.displayErrorMessage("An error occured while trying to create logging files, " +
					  "please check your files and try again");
	}
    }
    public Notification(Cal calendar, CalViewer calViewer) {
	this.calendar = calendar;
	this.calViewer = calViewer;
    }

    public void startNotificationTimer() {
	// Create a timer to check for upcoming appointments every minute
	timer = new Timer();
	timer.schedule(new TimerTask() {
	    @Override
	    public void run() {
		if (calendar.isEnabled()) {
		    checkAppointments();
		}
	    }
	}, 0, CHECK_RATE); // Check every minute
    }

    private void checkAppointments() { // Picky warning, the name is fine
	LocalTime currentTime = LocalTime.now();
	int currentMinutes = currentTime.getMinute() + currentTime.getHour() * HOUR_IN_MINUTES;  // convert the time into minutes for the calculations

	for (Appointment appointment: calendar.dayAppointments(currentDate.getDayOfMonth(), month, currentDate.getYear())) {
	    int appMinutes = appointment.getTimespan().getStart().getMinuteValue();  // Repetition because we need to use all these getters
	    int appHours = appointment.getTimespan().getStart().getHourValue();

	    // Calculate the time difference in minutes between the current time and the appointment start time
	    int timeDifference = (appHours * HOUR_IN_MINUTES + appMinutes) - currentMinutes;

	    // Check if the appointment starts within the next hour and hasn't been notified yet
	    if (timeDifference > 0 && timeDifference <= HOUR_IN_MINUTES && !notifiedAppointments.contains(appointment)) {
		playNotificationSound();
		calViewer.displayNotification(appointment.getSubject());
		notifiedAppointments.add(appointment);
	    }
	}
    }
    private void playNotificationSound() {
	// error message incase something goes wrong
	String audioErrorMessage = "Something went wrong while playing the notification sound," +
				   "please check your files";
	// try playing the sound from the specific filepath for the mp3 file
	try {
	    File soundFile = new File(System.getProperty("user.dir") + File.separator + "resources" + File.separator +
				      "audio" + File.separator + "notification.mp3");
	    try (FileInputStream fis = new FileInputStream(soundFile)) {
		Player player = new Player(fis);
		player.play();
	    } catch (IOException e) { // No fallthrough because the user is warned and crash occurs
		LOGGER.log(Level.SEVERE, "Make sure the audio file is correctly located in the audio folder", e);
		CalViewer.displayErrorMessage(audioErrorMessage);
	    }
	} catch (JavaLayerException e) { // No fallthrough because the user is warned and crash occurs
	    LOGGER.log(Level.SEVERE, "Make sure the files for the javazoom library are loaded correctly from the libs folder", e);
	    CalViewer.displayErrorMessage(audioErrorMessage);
	}
    }
}
