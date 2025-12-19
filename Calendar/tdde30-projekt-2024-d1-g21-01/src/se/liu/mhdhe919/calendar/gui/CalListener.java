package se.liu.mhdhe919.calendar.gui;

/**
 * Listener to update the displayed calendar each time it is modified
 * Gets notified from Cal by the sort() method
 */

public interface CalListener {
    public void calChanged();
}
