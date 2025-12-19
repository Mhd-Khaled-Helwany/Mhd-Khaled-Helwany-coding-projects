package se.liu.mhdhe919.calendar.time_units;

import java.util.Map;

/**
 * The class responsible for creating the month part of a CalDate datatype
 * Uses String datatype for the name and an int datatype for the month number
 * Contains 3 JavaMaps that supply the program with necessary information about the months of the year
 and because of that the use of magic numbers is necessary
 */

public class Month {
    private String name;
    private int number;

    public Month(final String name, final int number) {
	this.name = name;
	this.number = number;
    }

    public String getName() {
	return name;
    }

    public int getNumber() {
	return number;
    }

    /** Maps month names to the number of days in each month */
    private final static Map<String,Integer> MONTH_NAME_TO_LENGTH = Map.ofEntries(
	    Map.entry("january", 31),
	    Map.entry("february", 28),
	    Map.entry("march", 31),
	    Map.entry("april", 30),
	    Map.entry("may", 31),
	    Map.entry("june", 30),
	    Map.entry("july", 31),
	    Map.entry("august", 31),
	    Map.entry("september", 30),
	    Map.entry("october", 31),
	    Map.entry("november", 30),
	    Map.entry("december", 31)
    );
    /** Maps month names to their corresponding numbers */
    private final static Map<String,Integer> MONTH_NAME_TO_NUMBER = Map.ofEntries(
	    Map.entry("january", 1),
	    Map.entry("february", 2),
	    Map.entry("march", 3),
	    Map.entry("april", 4),
	    Map.entry("may", 5),
	    Map.entry("june", 6),
	    Map.entry("july", 7),
	    Map.entry("august", 8),
	    Map.entry("september", 9),
	    Map.entry("october", 10),
	    Map.entry("november", 11),
	    Map.entry("december", 12)
    );
    /** Maps month numbers to their corresponding names */
    private final static Map<Integer,String> MONTH_NUMBER_TO_NAME = Map.ofEntries(
	    Map.entry(1,"january"),
	    Map.entry(2,"february"),
	    Map.entry(3,"march" ),
	    Map.entry(4,"april"),
	    Map.entry(5,"may"),
	    Map.entry(6,"june"),
	    Map.entry(7,"july" ),
	    Map.entry(8,"august"),
	    Map.entry(9,"september"),
	    Map.entry(10,"october"),
	    Map.entry(11,"november" ),
	    Map.entry(12,"december")
    );
    /** Methods for retrieving important information from the maps */
    public static Integer getMonthDays(String name){
	return MONTH_NAME_TO_LENGTH.getOrDefault(name, -1);
    } // Explained in the report why an Integer is used
    public static Integer getMonthNumber(String name){
	return MONTH_NAME_TO_NUMBER.getOrDefault(name, -1);
    } // Explained in the report why an Integer is used
    public static String getMonthName(int number){return MONTH_NUMBER_TO_NAME.getOrDefault(number, "not found");}
}
