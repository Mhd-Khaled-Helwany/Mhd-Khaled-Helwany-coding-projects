package se.liu.mhdhe919.calendar.gui;

import se.liu.mhdhe919.calendar.Appointment;
import se.liu.mhdhe919.calendar.Cal;
import se.liu.mhdhe919.calendar.time_units.Month;

import javax.swing.*;
import java.awt.*;
import java.awt.event.ActionListener;
import java.util.List;
import java.awt.event.MouseAdapter;
import java.awt.event.MouseEvent;
import java.util.Objects;
import java.time.LocalDate;

/**
 * The class responsible for drawing the calendar so it can be displayed by CalViewer and other GUI components
 * Takes in a Cal instance to draw the unique calendar
 * Many magic numbers are present in this class due to each part of the GUI having unique positioning, size etc.
 */

public class CalComponent extends JComponent implements CalListener
{
    // Constants
    final private static int TILE_WIDTH_FACTOR = 7;
    /** Width factor for GUI components */
    final private static int TILE_HEIGHT_FACTOR = 15;
    /** Height factor for GUI components */

    // Instance variables
    private final Cal calendar;
    private String displayedMonth = "january";
    /** default starting month */
    private int displayedYear = LocalDate.now().getYear();

    /** default starting year */

    public CalComponent(final Cal calendar) {
	this.calendar = calendar;
	setLayout(null);
	// Register GUI components
	registerButtons();
	registerMouse();
    }

    /** Register GUI buttons */
    private void registerButtons() {
	// Create arrow buttons
	JButton prevMonthButton = new JButton("<html>&lt;</html>"); // < sign
	JButton nextMonthButton = new JButton("<html>&gt;</html>"); // > sign
	JButton prevYearButton = new JButton("<html>&lt;</html>"); // < sign
	JButton nextYearButton = new JButton("<html>&gt;</html>"); // > sign
	JButton showButton = new JButton("<html>Show</html>");
	JButton bookButton = new JButton("<html>Book</html>");
	JButton removeButton = new JButton("<html>Remove</html>");

	// Customize button appearance
	Dimension buttonSize = new Dimension(30, 30); // Set the size of the buttons
	prevMonthButton.setPreferredSize(buttonSize);
	nextMonthButton.setPreferredSize(buttonSize);
	prevYearButton.setPreferredSize(buttonSize);
	nextYearButton.setPreferredSize(buttonSize);
	showButton.setPreferredSize(buttonSize);
	bookButton.setPreferredSize(buttonSize);
	removeButton.setPreferredSize(buttonSize);

	// Set absolute positioning for buttons
	int monthButtonX = TILE_HEIGHT_FACTOR * 18;
	int monthButtonY = TILE_HEIGHT_FACTOR;
	int yearButtonX = TILE_HEIGHT_FACTOR * 46 + 5;
	int yearButtonY = TILE_WIDTH_FACTOR * 3;
	int calButtonX = TILE_HEIGHT_FACTOR * 10;
	int calButtonY = TILE_HEIGHT_FACTOR * 26 + 10;
	int buttonSpacing = TILE_HEIGHT_FACTOR * 13 + 5; // Spacing between buttons

	// Position and add buttons for month navigation
	prevMonthButton.setBounds(monthButtonX, monthButtonY, buttonSize.width, buttonSize.height);
	nextMonthButton.setBounds(monthButtonX + buttonSize.width + buttonSpacing, monthButtonY, buttonSize.width, buttonSize.height);

	// Position and add buttons for year navigation
	prevYearButton.setBounds(yearButtonX, yearButtonY, buttonSize.width - 10, buttonSize.height - 10);
	nextYearButton.setBounds(yearButtonX + buttonSize.width + buttonSpacing / 4 + 5, yearButtonY, buttonSize.width - 10,
				 buttonSize.height - 10);

	// Position and add buttons for showing, booking and removing
	showButton.setBounds(calButtonX, calButtonY, buttonSize.width * 2, buttonSize.height);
	bookButton.setBounds(calButtonX + buttonSpacing, calButtonY, buttonSize.width * 2, buttonSize.height);
	removeButton.setBounds(calButtonX + 2 * buttonSpacing, calButtonY, buttonSize.width * 2, buttonSize.height);

	// Add buttons to the component
	add(prevMonthButton);
	add(nextMonthButton);
	add(prevYearButton);
	add(nextYearButton);
	add(showButton);
	add(bookButton);
	add(removeButton);

	// Register listeners for other buttons
	registerShowButtonListener(showButton);
	registerBookButtonListener(bookButton);
	registerRemoveButtonListener(removeButton);

	// Register listeners for arrow buttons through the dedicated method
	registerArrowButtonListeners(prevMonthButton, nextMonthButton, prevYearButton, nextYearButton);
    }

    private void registerArrowButtonListeners(JButton prevMonthButton, JButton nextMonthButton, JButton prevYearButton,
					      JButton nextYearButton)
    {
	registerActionListener(prevMonthButton, e -> {
	    if (!(Objects.equals(displayedMonth, "january") && displayedYear == LocalDate.now().getYear())) {
		decrementMonth();
		calChanged();
	    }
	});

	registerActionListener(nextMonthButton, e -> {
	    if (!(Objects.equals(displayedMonth, "december") && displayedYear == LocalDate.now().getYear() + 10)) {
		incrementMonth();
		calChanged();
	    }
	});

	registerActionListener(prevYearButton, e -> {
	    decrementYear();
	    calChanged();
	});

	registerActionListener(nextYearButton, e -> {
	    incrementYear();
	    calChanged();
	});
    }

    private void registerShowButtonListener(JButton showButton) {
	registerActionListener(showButton, e -> {
	    // Create a custom input dialog for "Show" button
	    JPanel panel = new JPanel(new GridLayout(3, 2));
	    panel.add(new JLabel("Year:"));  // Duplicate code is better in this situation as trying to fix it will make more problems
	    JTextField yearField = new JTextField();
	    panel.add(yearField);
	    panel.add(new JLabel("Month:"));
	    JTextField monthField = new JTextField();
	    panel.add(monthField);
	    panel.add(new JLabel("Day:"));
	    JTextField dayField = new JTextField();
	    panel.add(dayField);

	    int result = JOptionPane.showConfirmDialog(null, panel, "Show", JOptionPane.OK_CANCEL_OPTION);
	    if (result == JOptionPane.OK_OPTION) {
		// Validate inputs
		if (!isValidNumber(yearField.getText()) || !isValidNumber(monthField.getText()) || !isValidNumber(dayField.getText())) {
		    CalViewer.displayErrorMessage("Please enter valid numbers for year, month, and day.");
		    return;
		}
		// Retrieve the inputs to show the appointments of the chosen day
		int day = Integer.parseInt(dayField.getText());
		int month = Integer.parseInt(monthField.getText());
		int year = Integer.parseInt(yearField.getText());

		calendar.showDay(day, month, year);
	    }
	});
    }

    private void registerBookButtonListener(JButton bookButton) {
	registerActionListener(bookButton, e -> {
	    // Create a custom input dialog for "Book" button
	    JPanel panel = new JPanel(new GridLayout(4, 3));
	    panel.add(new JLabel("Year:"));
	    JTextField yearField = new JTextField();
	    panel.add(yearField);
	    panel.add(new JLabel("Start Hour:"));
	    JTextField startHourField = new JTextField();
	    panel.add(startHourField);
	    panel.add(new JLabel("Month:"));
	    JTextField monthField = new JTextField();
	    panel.add(monthField);
	    panel.add(new JLabel("Start Minute:"));
	    JTextField startMinuteField = new JTextField();
	    panel.add(startMinuteField);
	    panel.add(new JLabel("Day:"));
	    JTextField dayField = new JTextField();
	    panel.add(dayField);
	    panel.add(new JLabel("End Hour:"));
	    JTextField endHourField = new JTextField();
	    panel.add(endHourField);
	    panel.add(new JLabel("Subject:"));
	    JTextField subjectField = new JTextField();
	    panel.add(subjectField);
	    panel.add(new JLabel("End Minute:"));
	    JTextField endMinuteField = new JTextField();
	    panel.add(endMinuteField);

	    int result = JOptionPane.showConfirmDialog(null, panel, "Book", JOptionPane.OK_CANCEL_OPTION);
	    if (result == JOptionPane.OK_OPTION) {
		// Validate inputs
		if (bookingRequirements(yearField, monthField, startHourField, startMinuteField, dayField, endHourField, endMinuteField)) {
		    CalViewer.displayErrorMessage(
			    "Please enter valid numbers for year, month, day, start hour, start minute, end hour, and end minute.");
		    return;
		}
		// retrieve the inputs to book the appointment
		int day = Integer.parseInt(
			dayField.getText());  // Duplicate code is better in this situation as trying to fix it will make more problems
		String month = Month.getMonthName(Integer.parseInt(monthField.getText()));
		int year = Integer.parseInt(yearField.getText());
		int startHour = Integer.parseInt(startHourField.getText());
		int startMinute = Integer.parseInt(startMinuteField.getText());
		int endHour = Integer.parseInt(endHourField.getText());
		int endMinute = Integer.parseInt(endMinuteField.getText());
		String subject = subjectField.getText();

		calendar.book(year, month, day, startHour, startMinute, endHour, endMinute, subject);
		calChanged();
	    }
	});
    }

    private void registerRemoveButtonListener(JButton removeButton) {
	registerActionListener(removeButton, e -> {
	    // Create a custom input dialog for "Remove" button
	    JPanel panel = new JPanel(new GridLayout(5, 2));
	    panel.add(new JLabel("Year:"));  // Duplicate code is better in this situation as trying to fix it will make more problems
	    JTextField yearField = new JTextField();
	    panel.add(yearField);
	    panel.add(new JLabel("Month:"));
	    JTextField monthField = new JTextField();
	    panel.add(monthField);
	    panel.add(new JLabel("Day:"));
	    JTextField dayField = new JTextField();
	    panel.add(dayField);
	    panel.add(new JLabel("Start Hour:"));
	    JTextField startHourField = new JTextField();
	    panel.add(startHourField);
	    panel.add(new JLabel("Start Minute:"));
	    JTextField startMinuteField = new JTextField();
	    panel.add(startMinuteField);

	    int result = JOptionPane.showConfirmDialog(null, panel, "Remove", JOptionPane.OK_CANCEL_OPTION);
	    if (result == JOptionPane.OK_OPTION) {
		// Validate inputs
		if (!isValidNumber(yearField.getText()) || !isValidNumber(monthField.getText()) || !isValidNumber(dayField.getText()) ||
		    !isValidNumber(startHourField.getText()) || !isValidNumber(startMinuteField.getText())) {
		    CalViewer.displayErrorMessage("Please enter valid numbers for year, month, day, start hour, and start minute.");
		    return;
		}
		// Retrieve the inputs to remove the appointment
		int year = Integer.parseInt(
			yearField.getText());  // Duplicate code is better in this situation as trying to fix it will make more problems
		String month = Month.getMonthName(Integer.parseInt(monthField.getText()));
		int day = Integer.parseInt(dayField.getText());
		int startHour = Integer.parseInt(startHourField.getText());
		int startMinute = Integer.parseInt(startMinuteField.getText());

		calendar.remove(year, month, day, startHour, startMinute);
		calChanged();
	    }
	});
    }

    private void registerActionListener(JButton button, ActionListener actionListener) {
	button.addActionListener(actionListener);
    }

    /** Requirements for a valid input */
    private boolean bookingRequirements(final JTextField yearField, final JTextField monthField, final JTextField startHourField,
					final JTextField startMinuteField, final JTextField dayField, final JTextField endHourField,
					final JTextField endMinuteField)
    {
	return !isValidNumber(yearField.getText()) || !isValidNumber(monthField.getText()) || !isValidNumber(startHourField.getText()) ||
	       !isValidNumber(startMinuteField.getText()) || !isValidNumber(dayField.getText()) || !isValidNumber(endHourField.getText()) ||
	       !isValidNumber(endMinuteField.getText());
    }

    private void incrementMonth() {
	int monthNumber = Month.getMonthNumber(displayedMonth);
	if (monthNumber == Month.getMonthNumber("december")) {
	    displayedMonth = "january";
	    incrementYear();
	} else {
	    displayedMonth = Month.getMonthName(monthNumber + 1);
	}
    }

    private void decrementMonth() {
	int monthNumber = Month.getMonthNumber(displayedMonth);
	if (monthNumber == Month.getMonthNumber("january")) {
	    displayedMonth = "december";
	    decrementYear();
	} else {
	    displayedMonth = Month.getMonthName(monthNumber - 1);
	}
    }

    private void incrementYear() {
	if (displayedYear < LocalDate.now().getYear() + 10) {
	    displayedYear++;
	}
    }

    private void decrementYear() {
	if (displayedYear > LocalDate.now().getYear()) {
	    displayedYear--;
	}
    }

    /** Method to check if a string is a valid number */
    private boolean isValidNumber(String input) {
	if (input == null || input.isEmpty()) {
	    return false;
	}
	for (char c : input.toCharArray()) {
	    if (!Character.isDigit(c)) {
		return false;
	    }
	}
	return true;
    }

    /** Register the clickable area of each rectangle in the GUI grid */
    private void registerMouse() {
	addMouseListener(new MouseAdapter()
	{
	    @Override public void mouseClicked(MouseEvent e) {
		int rectangleWidth = getWidth() / TILE_WIDTH_FACTOR;
		int rectangleHeight = getHeight() / TILE_HEIGHT_FACTOR;
		int offset = TILE_HEIGHT_FACTOR * 8;
		int column = e.getX() / rectangleWidth;
		int row = (e.getY() - offset) / rectangleHeight;
		int posYInRect = (e.getY() - offset) % rectangleHeight;
		if (posYInRect < 0) {
		    row--;
		}
		int day = row * TILE_WIDTH_FACTOR + column + 1;
		if (day >= 1 && day <= Month.getMonthDays(displayedMonth)) {
		    calendar.showDay(day, Month.getMonthNumber(displayedMonth), displayedYear);
		}
	    }
	});
    }

    @Override public Dimension getPreferredSize() {
	return new Dimension(800, 600);
    }

    /** Paint the GUI */
    @Override protected void paintComponent(Graphics g) {
	super.paintComponent(g);
	Graphics2D g2d = (Graphics2D) g;

	int rectangleWidth = getWidth() / TILE_WIDTH_FACTOR;
	int rectangleHeight = getHeight() / TILE_HEIGHT_FACTOR;
	int offset = TILE_HEIGHT_FACTOR * 8;
	int daysInMonth = Month.getMonthDays(displayedMonth);

	Font nameFont = g2d.getFont().deriveFont(Font.BOLD, 16.0f);  // Setting a font is necessary
	Font yearFont = g2d.getFont().deriveFont(Font.BOLD, 20.0f);
	Font monthFont = g2d.getFont().deriveFont(Font.BOLD, 30.0f);
	Font dayFont = g2d.getFont().deriveFont(Font.PLAIN, 12.0f); // Revert font size for day indicators
	Font appointmentFont = g2d.getFont().deriveFont(Font.PLAIN, 8.0f);

	// Set font for name display
	g2d.setFont(nameFont);
	g2d.setColor(Color.BLACK);
	g2d.drawString(String.valueOf(calendar.getName()), TILE_HEIGHT_FACTOR, rectangleHeight);

	// Set font for year display
	g2d.setFont(yearFont);
	g2d.setColor(Color.BLACK);
	g2d.drawString(String.valueOf(displayedYear), getWidth() - 80, rectangleHeight);
	String notificationIndicator = "Off";
	if (calendar.isEnabled()) {
	    notificationIndicator = "On";
	}
	g2d.drawString("Notifications: " + notificationIndicator, 575, 100);

	// Set font for month display
	g2d.setFont(monthFont);
	FontMetrics monthFontMetrics = g2d.getFontMetrics();  // Setting a font is necessary
	int stringWidthMonth = monthFontMetrics.stringWidth(displayedMonth);
	int monthCenterX = (getWidth() - stringWidthMonth) / 2;
	g2d.drawString(displayedMonth, monthCenterX, 40);

	for (int day = 1; day <= daysInMonth; day++) {
	    int row = (day - 1) / TILE_WIDTH_FACTOR;
	    int column = (day - 1) % TILE_WIDTH_FACTOR;

	    int x = column * rectangleWidth;
	    int y = row * rectangleHeight + offset;

	    // Draw day rectangle
	    g2d.setColor(Color.WHITE);
	    g2d.fillRect(x, y, rectangleWidth, rectangleHeight);
	    g2d.setColor(Color.BLACK);
	    g2d.drawRect(x, y, rectangleWidth, rectangleHeight);

	    // Draw day number
	    g2d.setFont(dayFont);
	    String dayNumber = String.valueOf(day);
	    int stringHeight = g2d.getFontMetrics().getHeight();
	    // Adjust position to top-left part of the rectangle
	    int topLeftX = x + 2; // 2-pixel padding from the left
	    int topLeftY = y + stringHeight - 2; // 2-pixel padding from the top
	    g2d.drawString(dayNumber, topLeftX, topLeftY);

	    // Retrieve appointments for the current day
	    List<Appointment> appointments = calendar.dayAppointments(day, displayedMonth, displayedYear);
	    // Display the first 2 appointments (if any)
	    int appointmentCount = Math.min(appointments.size(), 2);
	    for (int i = 0; i < appointmentCount; i++) {
		Appointment appointment = appointments.get(i);
		String startEndString = appointment.getTimespan().getStart() + "-" + appointment.getTimespan().getEnd() + " ";
		String subject = appointment.getSubject();

		// Truncate the subject if it's longer than 8 characters
		if (subject.length() > 8) {
		    subject = subject.substring(0, 8) + "...";
		}

		String appointmentString = startEndString + subject;
		// Adjust position to prevent overlapping with day number
		g2d.setFont(appointmentFont);
		g2d.drawString(appointmentString, x + 20, y + stringHeight + i * 10);
	    }
	    // If there are more than 2 appointments, display a message
	    if (appointments.size() > 2) {
		g2d.drawString("Click to show more", x + 10, y + stringHeight + 2 * 10);
	    }

	}
    }

    /** Calendar changed, time to update the window */
    @Override public void calChanged() {
	repaint();
    }
}