package se.liu.mhdhe919.calendar.time_units;

/**
 * The class responsible for creating the minute part of a TimePeriod datatype
 * Uses an int datatype to represent the minute value
 */

public class Minute {
    private int minute;

    public Minute(int minute) {
        this.minute = minute;
    }

    public int getMinute() {
        return minute;
    }
}
