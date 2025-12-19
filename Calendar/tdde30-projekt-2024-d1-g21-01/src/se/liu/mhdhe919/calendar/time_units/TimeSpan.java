package se.liu.mhdhe919.calendar.time_units;

/**
 * The class responsible for creating the timespan part of the Appointment datatype
 * Combines two TimePeriod datatypes to represent a timespan
 */

public class TimeSpan {
    private TimePeriod start;
    private TimePeriod end;
    public TimeSpan(final TimePeriod start, final TimePeriod end) {
        this.start = start;
        this.end = end;
    }

    public TimePeriod getStart() {
        return start;
    }

    public TimePeriod getEnd() {
        return end;
    }

    /** Checks if this time span overlaps with another time span */
    public boolean overlap(TimeSpan other){
        if (this.getStart().periodPrecedes(other.getEnd()) &&
            other.getStart().periodPrecedes(this.getEnd())){
            return true;
        }
        else{
            return false;
        }
    }
    @Override public String toString(){
        return getStart() + "-" + getEnd();

    }
}
