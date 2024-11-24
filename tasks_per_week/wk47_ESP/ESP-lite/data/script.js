const compassControl = document.getElementById("compassControl");
const compassValue = document.getElementById("compassValue");
const defaultValue = 0; // Default compass value

// Set initial compass value
updateCompass(defaultValue);
sendCompassValue(defaultValue);
drawLineToCircumference(defaultValue, compassControl.getBoundingClientRect());


// Update the compass display
function updateCompass(pos) {
    compassValue.innerText = `${pos}°`; // Update compass value display
}

// Send the current compass value to the server
function sendCompassValue(pos) {
    fetch(`/compass?value=${pos}`); // Send the compass value to the server
    console.log("Compass value", pos); // Log the value for debugging
}



// Utility to calculate the angle relative to the center
function calculateAngleFromCenter(clientX, clientY, rect) {
	const centerX = rect.left + rect.width / 2;
	const centerY = rect.top + rect.height / 2;
	const dx = centerX - clientX;
	const dy = centerY - clientY; 
	let angle = Math.atan2(dy, dx) * (180 / Math.PI);
	angle = (angle + 360 - 90) % 360; 
	return Math.round(angle);
}

// Function to draw a line from the center to the circumference
function drawLineToCircumference(angle, rect) {
    const centerX = rect.width / 2;
    const centerY = rect.height / 2;
    const radius = Math.min(rect.width, rect.height) / 2;
    const canvas = document.getElementById("compassCanvas");
    const ctx = canvas.getContext("2d");

    // Clear the canvas before drawing
    ctx.clearRect(0, 0, canvas.width, canvas.height);

    // Convert the angle to radians and calculate the end point
    const radians = (angle - 90) * (Math.PI / 180);
    const endX = centerX + radius * Math.cos(radians);
    const endY = centerY + radius * Math.sin(radians);

    ctx.beginPath();
    ctx.moveTo(centerX, centerY);
    ctx.lineTo(endX, endY);
    ctx.strokeStyle = "red";
    ctx.lineWidth = 2;
    ctx.stroke();
}

// Handle real-time updates during interaction
function updateCompassOnMove(event) {
	event.preventDefault();
	const rect = compassControl.getBoundingClientRect();
    const clientX = event.touches ? event.touches[0].clientX : event.clientX;
    const clientY = event.touches ? event.touches[0].clientY : event.clientY;

    const angle = calculateAngleFromCenter(clientX, clientY, rect);
    updateCompass(angle);
    drawLineToCircumference(angle, rect);
}

// Handle the final action when releasing the interaction
function finalizeCompassInteraction(event) {
	const rect = compassControl.getBoundingClientRect();
    const clientX = event.changedTouches ? event.changedTouches[0].clientX : event.clientX;
    const clientY = event.changedTouches ? event.changedTouches[0].clientY : event.clientY;

    const angle = calculateAngleFromCenter(clientX, clientY, rect);
    sendCompassValue(angle);
}



let isMouseDown = false;

function startInteraction(event) {
    isMouseDown = true;
    updateCompassOnMove(event);
}

function endInteraction(event) {
    isMouseDown = false;
    finalizeCompassInteraction(event);
}

function moveInteraction(event) {
    if (isMouseDown) {
        updateCompassOnMove(event);
    }
}

// Mouse events
compassControl.onmousedown = startInteraction;
compassControl.onmouseup = endInteraction;
compassControl.onmousemove = moveInteraction;

// Touch events
compassControl.ontouchstart = startInteraction;
compassControl.ontouchend = endInteraction;
compassControl.ontouchmove = moveInteraction;


// Functions for moving the motor forward and backward with specific distances
function forwards5() { move("forwards", 5); } 		// Calling move function with separated parameters
function forwards20() { move("forwards", 20); }		// Calling move function with separated parameters
function backwards5() { move("backwards", 5); }		// Calling move function with separated parameters
function backwards20() { move("backwards", 20); }	// Calling move function with separated parameters


// This is a general-purpose function that can handle different combinations of direction and distance
function move(dir, dis) { 
	fetch(`/${dir}${dis}`);			// Sends a request to the server with a URL constructed from the received direction and distance (e.g., /forwards5)
	console.log("Drive", dir, dis);	// Log the movement command to the console for debugging
}

