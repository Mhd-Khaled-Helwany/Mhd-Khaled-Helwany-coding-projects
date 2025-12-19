package se.liu.mhdhe919.calendar.time_units;

/**
 * The class responsible for creating the timeperiods for the TimeSpan datatype
 * Combines an Hour datatype and a minute datatype to represent the time
 */

public class TimePeriod {
    private Hour hour;
    private Minute minute;

    public TimePeriod(Hour hour, Minute minute) {
        this.hour = hour;
        this.minute = minute;
    }
    public int getHourValue() {
        return hour.getHour();
    }

    public int getMinuteValue() {
        return minute.getMinute();
    }

    /** Checks if this time period precedes another time period */
    public boolean periodPrecedes(TimePeriod other){
        if (this.getHourValue() != other.getHourValue()){
            return this.getHourValue() < other.getHourValue();
        }
        else{
            return this.getMinuteValue() < other.getMinuteValue();
        }
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;
        TimePeriod that = (TimePeriod) o;
        return getHourValue() == that.getHourValue() && getMinuteValue() == that.getMinuteValue();
    }
    @Override public String toString(){
        return String.format("%02d:%02d", getHourValue(), getMinuteValue());

    }

}
