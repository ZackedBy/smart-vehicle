// ----------------------------- COMPASS RELATED -----------------------------

const compassControl = document.getElementById("compassControl");
const compassValue = document.getElementById("compassValue");
const compassText = document.getElementById("compassText");
const compassForm = document.getElementById("compassForm");
const compassInput = document.getElementById("compassInput");
const compassWheel = document.getElementById("compassWheel")
const defaultValue = 0;
// Variable to store the last valid compass value
let lastValidCompassValue;

// Function to set compass to 0. Used on load and for Find North button
function setCompassTo(value) {
  updateCompass(value);
  sendCompassValue(value);
}

setCompassTo(defaultValue); // Set compass to zero on load

// Update the compass display
function updateCompass(pos) {
  compassText.innerText = `${pos}°`; // Update compass value display
  compassInput.value = pos;
  compassWheel.style.transform = `rotate(${pos}deg)`;
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

const forwardInput = document.getElementById("forwardInput");
const backwardsInput = document.getElementById("backwardInput");
const pedals = document.getElementsByClassName("pedal");

// Function to validate input and enable/disable the button
function validateInput(input, pedal) {
  // Check if the value is a valid number
  if (isNaN(input.value) || input.value === "" || input.value <= 0) {
    input.style.backgroundColor = "red";
    pedal.style.pointerEvents = "none"; // Disable interactions
  } else {
    input.style.backgroundColor = "";
    pedal.style.pointerEvents = "auto"; // Enable interactions
  }
}

// Event listeners for input change
forwardInput.addEventListener("input", () =>
  validateInput(forwardInput, pedals[1])
);
backwardsInput.addEventListener("input", () =>
  validateInput(backwardsInput, pedals[0])
);

function pressPedal(event) {
  if (event.target.style.pointerEvents === "none") return;
  event.target.classList.add("pressed");
}

function releasePedal(event) {
  if (event.target.style.pointerEvents === "none") return;
  event.target.classList.remove("pressed");
}

// Mouse Events
for (let i = 0; i < pedals.length; i++) {
  pedals[i].addEventListener("mousedown", pressPedal);
  pedals[i].addEventListener("mouseup", releasePedal);
}

// Touch Events for Mobile
for (let i = 0; i < pedals.length; i++) {
  pedals[i].addEventListener("touchstart", pressPedal);
  pedals[i].addEventListener("touchend", releasePedal);
}

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

//------------------------LOGOUT-----------------------------------------
function logout() {
  sessionStorage.clear();
  window.location.href = "/";
  return false;
}

//------------------GET VALUES FROM ESP--------------------------

const realCompassValue = document.getElementById("realCompassValue");
const lidarText = document.getElementById("lidar-val");
const colorBall = document.getElementById("Color");

async function fetchEspThenUpdate() {
  try {
    const response = await fetch("/getData");
    if (!response.ok) throw new Error(`HTTP error! Status: ${response.status}`);

    const data = await response.json();

    // Update compass value
    if (data.compass) {
      const compassValue = parseInt(data.compass, 10);
      if (!isNaN(compassValue)) {
        compassText.innerText = `${compassValue}°`;
        //compassInput.value = compassValue;
      }
    }

    // Updates lidar values
    if (data.lidar) {
      const lidarValue = parseInt(data.lidar, 10);
      if (!isNaN(lidarValue)) {
        if (lidarValue <= 10) {
          lidarText.style.color = "red";
        } else if (lidarValue > 10 && lidarValue <= 30) {
          lidarText.style.color = "orange";
        } else {
          lidarText.style.color = "white";
        }
        lidarText.innerText = `${lidarValue} cm`;
      }
    }

    // Updates color value
    if (data.color) {
      colorBall.style.setProperty("background-color", data.color, "important");
    }
  } catch (error) {
    console.error("Error fetching data:", error);
  }
}

setInterval(fetchEspThenUpdate, 50);
