//------------------------LOGOUT-----------------------------------------

// Logs out via button
async function logout() {
  try {
    // Send GET request to logout route
    const response = await fetch("/logout", { method: "GET" });

    if (response.ok) {
      // Redirect to login page after logout
      window.location.href = "/";  // Redirect to the login page
    } else {
      console.error("Logout failed");
    }
  } catch (error) {
    console.error("Logout error:", error);
  }
}

// Logout on closing page so bool flag in ESP resets
window.addEventListener("beforeunload", function() {
  //e.preventDefault();
  fetch("/logout", { method: "GET", keepalive: true})
})

// ----------------------------- COMPASS RELATED -----------------------------

const compassValue = document.getElementById("compassValue");
const compassText = document.getElementById("compassText");
const compassForm = document.getElementById("compassForm");
const compassInput = document.getElementById("compassInput");

const defaultValue = 0;
// Variable to store the last valid compass value
let lastValidCompassValue;

// Function to set compass to 0. Used on load and for Find North button
function setCompassTo(val) {
  //updateCompass(value);
  compassInput.value = val;
  sendCompassValue(val);
}

setCompassTo(defaultValue); // Set compass to zero on load

// Update the compass display
function updateCompass(pos) {
  compassText.innerText = `${pos}°`; // Update compass value display
  compassInput.value = pos;
}

// Send the current compass value to the server
async function sendCompassValue(pos) {
  lastValidCompassValue = pos;
  try {
    const response = await fetch("/compass", {
      method: "POST",
      headers: {
        "Content-Type": "application/x-www-form-urlencoded"
      },
      body: `value=${pos}`
    });
    console.log(`Compass value: ${pos}; `, response.status);
  } catch (error) {
    console.error("Error sending turning command:", error);
  }
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
    compassForm.contains(e.target) || compassValue.contains(e.target);

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

// ----------------------------- MOVEMENT RELATED -----------------------------


// This is a general-purpose function that can handle different combinations of direction and distance
async function move() {

  try {
    const response = await fetch("/move", {
      method: "POST",
      headers: {
        "Content-Type": "application/x-www-form-urlencoded"
      },
      body: `distance=${20}`
    });
    console.log(`Move: ${20}; `, response.status);
  } catch (error) {
    console.error("Error sending move command:", error);
  }
}

//------------------GET VALUES FROM ESP--------------------------
const lidarText = document.getElementById("lidar-val");
const colorBall = document.getElementById("Color");
const encVal = document.getElementById("encVal");

async function fetchEspThenUpdate() {
  try {
    const response = await fetch("/getData", { method: "GET" });
    if (!response.ok) throw new Error(`HTTP error! Status: ${response.status}`);

    const data = await response.json();

    // Update compass value only if user isnt moving wheel
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

    if (data.enc) {
      encVal.innerText = data.enc;
    }

  } catch (error) {
    console.error("Error fetching data:", error);
  }
}

setInterval(fetchEspThenUpdate, 100);


async function startCalibrate() {
  try {
    const response = fetch("/enc", { method: "GET" });
  } catch (error) {
    console.error("Huh?")
  }
}