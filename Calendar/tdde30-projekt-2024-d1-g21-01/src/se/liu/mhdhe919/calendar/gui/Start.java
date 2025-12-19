package se.liu.mhdhe919.calendar.gui;

import javax.swing.*;
import java.awt.*;

/**
 * The class responsible for displaying the welcome image as a startscreen for the user
 * Uses a Dimension datatype to set the size of the image
 */

public class Start extends JComponent {

    private final ImageIcon icon;

    public Start(Dimension size) {
	// retrieve the image from the images directory and give it the desired size
	icon = new ImageIcon(ClassLoader.getSystemResource("images/Welcome_Calendar_Logo.png"));
	setPreferredSize(size);
    }

    /** Display the welcome image */
    @Override protected void paintComponent(Graphics g) {
	super.paintComponent(g);
	Graphics2D g2d = (Graphics2D) g;
	g2d.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_ON);

	int x = (getWidth() - icon.getIconWidth()) / 2;
	int y = (getHeight() - icon.getIconHeight()) / 2;

	icon.paintIcon(this, g, x, y);
    }
}
