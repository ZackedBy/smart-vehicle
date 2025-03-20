// ----------------------------- COMPASS RELATED -----------------------------

// DOM Elements for Compass Control
const compassControl = document.getElementById("compassControl");
const compassValue = document.getElementById("compassValue");
const realCompassValue = document.getElementById("realCompassValue");
const compassText = document.getElementById("compassText");
const compassForm = document.getElementById("compassForm");
const compassInput = document.getElementById("compassInput");
const defaultValue = 0;
// Variable to store the last valid compass value
let lastValidCompassValue;

// This function fetches asynchronously LIDAR data from the server and updates the display.
async function fetchCompassData() {
  try {
    const response = await fetch("/realComp"); // Fetch the LIDAR data from the server
    const data = await response.text(); // Get the response text
    const compVal = parseInt(data, 10); // Parse the LIDAR data as an integer

    if (compVal) {
        //console.log("Lidar value:", lidarValue);
        realCompassValue.innerText = `${compVal}°`;  // Update the LIDAR text element with the fetched data
     }
  } catch (error) {
    console.error("COMP ERR:", error); // Log any errors to the console
  }
}

setInterval(fetchCompassData, 50);


// Function to set compass to 0. Used on load and for Find North button
function setCompassTo(value) {
  updateCompass(value);
  sendCompassValue(value);
  //drawLineToCircumference(value, compassControl.getBoundingClientRect());
}

setCompassTo(defaultValue); // Set compass to zero on load

// Update the compass display
function updateCompass(pos) {
  compassText.innerText = `${pos}°`; // Update compass value display
  compassInput.value = pos;
  rotateCompassWheel(pos);
}

function rotateCompassWheel(pos) {
  let compass = document.getElementById("compassWheel");
  compass.style.transform = `rotate(${pos}deg)`;
}

// Send the current compass value to the server
async function sendCompassValue(pos) {
  lastValidCompassValue = pos;
  try {
    const response = await fetch(`/compass?value=${pos}`);
    console.log(`Compass value: ${pos}; `, response.status);
  } catch (error) {
    console.error("Error sending turning command:", error);
  }
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
/*function drawLineToCircumference(angle, rect) {
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
}*/

// Handle real-time updates during interaction
function updateCompassOnMove(event) {
  event.preventDefault();
  const rect = compassControl.getBoundingClientRect();
  const clientX = event.touches ? event.touches[0].clientX : event.clientX;
  const clientY = event.touches ? event.touches[0].clientY : event.clientY;

  const angle = calculateAngleFromCenter(clientX, clientY, rect);
  updateCompass(angle);
  //drawLineToCircumference(angle, rect);
}

// Handle the final action when releasing the interaction
function finalizeCompassInteraction(event) {
  const rect = compassControl.getBoundingClientRect();
  const clientX = event.changedTouches
    ? event.changedTouches[0].clientX
    : event.clientX;
  const clientY = event.changedTouches
    ? event.changedTouches[0].clientY
    : event.clientY;

  const angle = calculateAngleFromCenter(clientX, clientY, rect);
  sendCompassValue(angle);
}

// Toggle input field visibility for editing compass value
function editCompassValue() {
  compassText.style.display = "none";
  compassForm.style.display = "inline";
  compassInput.focus();
}

// Handle form submission to update compass value
compassForm.addEventListener("submit", (e) => {
  e.preventDefault();

  // Check if the input is empty, and revert to the last valid value if so
  let angle = parseInt(compassInput.value);
  if (isNaN(angle) || compassInput.value === "") {
    angle = lastValidCompassValue; // If input is empty or invalid, use the last valid value
  } else {
    // Ensure the value is within the valid range of 0 to 360
    angle = Math.min(Math.max(angle, 0), 360);
  }

  // Update the compass display and draw the line
  setCompassTo(angle);

  // Update last valid value
  lastValidCompassValue = angle;

  // Revert back to text view after submission
  compassText.style.display = "inline";
  compassForm.style.display = "none";
});

// Close the form when clicking outside of it
document.addEventListener("click", function (e) {
  const isClickInside =
    compassForm.contains(e.target) || compassText.contains(e.target);

  // If clicked outside and the form is visible
  if (!isClickInside && compassForm.style.display === "inline") {
    // Check if the compassInput is empty
    if (compassInput.value === "") {
      // Revert to the last valid value if empty
      compassInput.value = lastValidCompassValue;
    }
    // Trigger form submission
    compassForm.requestSubmit(); // This will submit the form
  }
});

// Mouse and touch events for compass interaction
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

// ----------------------------- MOVEMENT RELATED -----------------------------

// DOM Elements for Movement Control
const forwardInput = document.getElementById("forwardInput");
const backwardsInput = document.getElementById("backwardInput");
const forwardButton = document.querySelector('button[onclick="forwards()"]');
const backwardsButton = document.querySelector('button[onclick="backwards()"]');

// Function to validate input and enable/disable the button
function validateInput(input, button) {
  // Check if the value is a valid number
  if (isNaN(input.value) || input.value === "") {
    input.style.backgroundColor = "red"; // Change the background color to red for invalid input
    button.style.backgroundColor = "red"; // Change the button color to red
    button.style.color = "white";
    button.style.cursor = "default";
    button.disabled = true; // Disable the button if the input is not valid
  } else {
    input.style.backgroundColor = ""; // Reset the background color if input is valid
    button.style.backgroundColor = "#46ff46"; // Green button when valid
    button.style.color = "#555555";
    button.style.cursor = "pointer";
    button.disabled = false; // Enable the button if the input is valid
  }
}

// Event listeners for input change
forwardInput.addEventListener("input", () =>
  validateInput(forwardInput, forwardButton)
);
backwardsInput.addEventListener("input", () =>
  validateInput(backwardsInput, backwardsButton)
);

// Function for moving the motor forward with specific distance
function forwards() {
  const value = forwardInput.value;
  move("forwards", value);
}

// Function for moving the motor backward with specific distance
function backwards() {
  const value = backwardsInput.value;
  move("backwards", value);
}

// This is a general-purpose function that can handle different combinations of direction and distance
function move(dir, dis) {
  fetch(`/${dir}?value=${dis}`)
    .then((response) =>
      console.log(`Drive: ${dir} ${dis}cm; `, response.status)
    )
    .catch((error) => console.error("Error sending move command:", error));
}

// ----------------------------- LIDAR RELATED -----------------------------
const lidarText = document.getElementById("lidar-val");

// This function fetches asynchronously LIDAR data from the server and updates the display.
async function fetchLidarData() {
  try {
    const response = await fetch("/lidar"); // Fetch the LIDAR data from the server
    const data = await response.text(); // Get the response text
    const lidarValue = parseInt(data, 10); // Parse the LIDAR data as an integer

    if (lidarValue) {
        //console.log("Lidar value:", lidarValue);
        lidarText.innerText = `${lidarValue} cm`;  // Update the LIDAR text element with the fetched data
     }
  } catch (error) {
    console.error("Error fetching LIDAR data:", error); // Log any errors to the console
  }
}

setInterval(fetchLidarData, 50);

//--------------------------COLOR SENSOR---------------------------
//Function to fetch and display the closest color the sensor detects on the ground
const colorBall = document.getElementById("Color");
async function fetchColorData() {
  try {
    const response = await fetch("/rgb");
    const color = await response.text();

    colorBall.style.setProperty("background-color", color, "important");

  } catch (error) {
    console.log("Error fetching color data", error)
  }
}
setInterval(fetchColorData, 50);

//----------------------INTRO VIDEO-------------------------------
// document.addEventListener("DOMContentLoaded", function() {
//   const video = document.getElementById("intro");

//   video.addEventListener("ended", function() {
//     video.style.display = "none";
//   });

//   video.addEventListener("click", () => video.style.display = "none");
//   document.addEventListener("keydown", () => video.style.display = "none");
// });

//------------------------LOGOUT-----------------------------------------
async function logout() {
  // fetch("/logout")
  // setTimeout(() => {
  //   window.location.href = "/";
  // }, 1000);
}