package se.liu.mhdhe919.calendar.time_units;

/**
 * The class responsible for creating the date part of an Appointment datatype
 * Uses 2 int datatypes for the day and year and a Month datatype for the month
 */

public class CalDate
{
    private int day;
    private Month month;
    private int year;

    public CalDate(final int day, final Month month, final int year) {
	this.day = day;
	this.month = month;
	this.year = year;
    }

    public int getDay() {
	return day;
    }

    public Month getMonth() {
	return month;
    }

    public int getYear() {
	return year;
    }
    @Override public String toString(){

	return getDay() + " " + month.getName() + " " + getYear();
    }
    @Override
    public boolean equals(Object o) {
	if (this == o) return true;
	if (o == null || getClass() != o.getClass()) return false;
	CalDate that = (CalDate) o;
	return year == that.year && month.getNumber() == that.month.getNumber() && day == that.day;
    }

}
