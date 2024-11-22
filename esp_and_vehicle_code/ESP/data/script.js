const compassSlider = document.getElementById('compassSlider'); // Get the compass slider element from the HTML page


// Check if the compassSlider element exists and reset it to 0
if (compassSlider) {
	const middleValue = 180;
	compassSlider.value = middleValue;	// Set slider to the middle position (0 degrees)
	updateCompass(middleValue);			// Set the compass value to initial value of 0	
}


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
// function forwards(value) {
// 	move('forwards', value)
// };


// This is a general-purpose function that can handle different combinations of direction and distance
function move(dir, dis) { 
	fetch(`/${dir}${dis}`);			// Sends a request to the server with a URL constructed from the received direction and distance (e.g., /forwards5)
	console.log("Drive", dir, dis);	// Log the movement command to the console for debugging
}

