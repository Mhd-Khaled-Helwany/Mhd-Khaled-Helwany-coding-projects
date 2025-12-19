package se.liu.mhdhe919.calendar;

import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import com.google.gson.JsonObject;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/**
 * The class responsible for saving and reading calendar files in the json formalt
 * Takes in a Cal instance to save it's contents or modify it with a existing calendar.json file
 */

public class CalFile {
    private Cal calendar;

    public CalFile(final Cal calendar) {
	this.calendar = calendar;
    }

    public void saveCal() throws IOException {
	// Initialize the json file and define the data that will be stored in it
	List<Appointment> calAppointments = calendar.getAppointments();
	Gson gson = new GsonBuilder().setPrettyPrinting().create();
	String appointmentList = gson.toJson(calAppointments);  // Name is fine
	String calDataJson = "{\"name\": \"" + calendar.getName() + "\", \"appointments\": " + appointmentList + "}";
	String tempFilePath =  System.getProperty("user.dir") + File.separator + "temp_calendar.json"; // Pointless repeated expression warning, one time repeat
	String filePath = System.getProperty("user.dir") + File.separator + "calendar.json";

	// Write to a temporary file
	try (FileWriter writer = new FileWriter(tempFilePath)) {
	    writer.write(calDataJson);
	}

	// If writing to temporary file was successful, proceed to rename
	File mainFile = new File(filePath);

	// Delete the old main file
	Files.deleteIfExists(mainFile.toPath());

	// Rename the temporary file to the name of the main file
	Files.move(Paths.get(tempFilePath), Paths.get(filePath));
    }
    public void readCal() throws IOException, FileNotFoundException {
	String filePath = System.getProperty("user.dir") + File.separator + "calendar.json";
	try (FileReader reader = new FileReader(filePath)) {
	    Gson gson = new Gson();
	    JsonObject calDataJson = gson.fromJson(reader, JsonObject.class);

	    // Extract calendar name and set it
	    String calName = calDataJson.get("name").getAsString();
	    calendar.setName(calName);

	    // Extract appointments array and set it
	    Appointment[] appointments = gson.fromJson(calDataJson.get("appointments"), Appointment[].class);
	    List<Appointment> listOfAppointments = new ArrayList<>(Arrays.asList(appointments)); // Name is fine
	    calendar.setAppointments(listOfAppointments);
	}
    }
}
