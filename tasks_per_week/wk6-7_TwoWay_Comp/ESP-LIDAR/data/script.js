const compassSlider = document.getElementById('compassSlider'); // Get the compass slider element from the HTML page
const lidarText = document.getElementById('lidarText'); // // Get the LIDAR text element from the HTML page

// Check if the compassSlider element exists and reset it to 0
if (compassSlider) {
	const middleValue = 0;
	compassSlider.value = middleValue;	// Set slider to the middle position (0 degrees)
	updateCompass(middleValue);			// Set the compass value to initial value of 0	
}


setInterval(fetchLidarData, 500); // Fetch LIDAR data every half second

// Function to update the compass display (show the current compass position)
function updateCompass(pos) {
	document.getElementById('compassValue').innerText = `${pos}°`;// Update the text value for compassValue when value is changing on page element with the current position and add "°" for degrees
}


// Function to send the current compass value to the server
function sendCompassValue(pos) {
	fetch(`/compass?value=${pos}`);		// Send the compass value to the server using a fetch request
	console.log("Compass value", pos);	// Log the compass value to the console for debugging
}


// Functions for moving the motor forward and backward with specific distances
function forwards5() { move('forwards', 5); } 		// Calling move function with separated parameters
function forwards20() { move('forwards', 20); }		// Calling move function with separated parameters
function backwards5() { move('backwards', 5); }		// Calling move function with separated parameters
function backwards20() { move('backwards', 20); }	// Calling move function with separated parameters


// This is a general-purpose function that can handle different combinations of direction and distance
function move(dir, dis) { 
	fetch(`/${dir}${dis}`);			// Sends a request to the server with a URL constructed from the received direction and distance (e.g., /forwards5)
	console.log("Drive", dir, dis);	// Log the movement command to the console for debugging
}


// This function fetches asynchronously LIDAR data from the server and updates the display.
async function fetchLidarData() {
  try {
    const response = await fetch('/lidar');		// Fetch the LIDAR data from the server
    const data = await response.text();			// Get the response text
    const lidarValue = parseInt(data, 10); 		// Parse the LIDAR data as an integer

    if (lidarText) lidarText.innerText = `${lidarValue} cm`;	// Update the LIDAR text element with the fetched data
  } catch (error) {
    console.error('Error fetching LIDAR data:', error);		 	// Log any errors to the console
  }
}