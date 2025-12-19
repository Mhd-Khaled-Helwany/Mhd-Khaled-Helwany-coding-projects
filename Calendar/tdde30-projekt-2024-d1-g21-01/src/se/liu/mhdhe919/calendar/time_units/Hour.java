package se.liu.mhdhe919.calendar.time_units;

/**
 * The class responsible for creating the hour part of a TimePeriod datatype
 * Uses an int datatype to represent the hour value
 */

public class Hour {

    private int hour;

    public Hour(int hour) {
        this.hour = hour;
    }

    public int getHour() {
        return hour;
    }
}
