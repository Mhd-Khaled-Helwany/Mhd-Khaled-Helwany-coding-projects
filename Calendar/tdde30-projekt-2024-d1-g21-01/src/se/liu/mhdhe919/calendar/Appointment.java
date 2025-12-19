package se.liu.mhdhe919.calendar;

import se.liu.mhdhe919.calendar.time_units.CalDate;
import se.liu.mhdhe919.calendar.time_units.TimeSpan;

/**
 * The class responsible for creating the appointments of a Calendar datatype
 * Uses a String, a CalDate and a Timespan datatypes for the subject, date
 and timespan parts respectively
 */
public class Appointment {
    private String subject;
    private CalDate date;
    private TimeSpan timespan;

    public Appointment(final String subject, final CalDate date, final TimeSpan timespan) {
	this.subject = subject;
	this.date = date;
	this.timespan = timespan;
    }

    public String getSubject() {
	return subject;
    }

    public CalDate getDate() {
	return date;
    }

    public TimeSpan getTimespan() {
	return timespan;
    }

    @Override public String toString(){
	return "Appointment: " + getSubject() + "\nDate: " + getDate() + "\nTime: " + getTimespan();
    }
}
