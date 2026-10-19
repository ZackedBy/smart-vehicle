const knownColors = ["red", "green", "blue", "black"];

const logoutButton = document.getElementById("logout-button");
const mazeButton = document.getElementById("maze-button");
const stopButton = document.getElementById("stop-button");
const moveButton = document.getElementById("move-button");
const calibrateButton = document.getElementById("calibrate-button");
const distanceInput = document.getElementById("distance-input");
const statusText = document.getElementById("status");
const colorSwatch = document.getElementById("color-swatch");
const encVal = document.getElementById("enc-val");
const compassValue = document.getElementById("compass-value");
const compassText = document.getElementById("compass-text");
const compassForm = document.getElementById("compass-form");
const compassInput = document.getElementById("compass-input");
const lidarText = document.getElementById("lidar-val");

let lastAngle = 0;
let editingCompass = false;

function setStatus(text) {
  if (statusText) statusText.textContent = text;
}

async function postForm(path, body) {
  const response = await fetch(path, {
    method: "POST",
    headers: { "Content-Type": "application/x-www-form-urlencoded" },
    body
  });
  if (response.status === 401) {
    window.location.href = "/";
    return null;
  }
  if (!response.ok) throw new Error("HTTP " + response.status);
  return response;
}

async function logout() {
  try {
    await fetch("/logout");
  } catch (err) {
    console.error(err);
  }
  window.location.href = "/";
}

function showCompass(angle) {
  lastAngle = angle;
  compassText.textContent = angle + "\u00b0";
  if (!editingCompass) compassInput.value = String(angle);
}

async function sendAngle(angle) {
  const clamped = Math.min(360, Math.max(0, angle));
  showCompass(clamped);
  await postForm("/compass", new URLSearchParams({ value: String(clamped) }));
  setStatus("Turn " + clamped);
}

function closeCompassEditor() {
  editingCompass = false;
  compassText.style.display = "inline";
  compassForm.style.display = "none";
}

logoutButton.addEventListener("click", logout);
mazeButton.addEventListener("click", async () => {
  try {
    await postForm("/maze");
    setStatus("Maze");
  } catch (err) {
    setStatus("Maze command failed");
  }
});
stopButton.addEventListener("click", async () => {
  try {
    await postForm("/stop");
    setStatus("Stopped");
  } catch (err) {
    setStatus("Stop failed");
  }
});
moveButton.addEventListener("click", async () => {
  const distance = Number.parseInt(distanceInput.value, 10);
  if (!Number.isInteger(distance) || distance < 1 || distance > 500) {
    setStatus("Distance must be 1-500 cm");
    return;
  }
  try {
    await postForm("/move", new URLSearchParams({ distance: String(distance) }));
    setStatus("Drive " + distance + " cm");
  } catch (err) {
    setStatus("Drive failed");
  }
});
calibrateButton.addEventListener("click", async () => {
  try {
    await postForm("/enc");
    setStatus("Calibrating encoder");
  } catch (err) {
    setStatus("Calibrate failed");
  }
});

compassValue.addEventListener("click", (event) => {
  if (compassForm.contains(event.target)) return;
  editingCompass = true;
  compassText.style.display = "none";
  compassForm.style.display = "inline";
  compassInput.focus();
  compassInput.select();
});

compassForm.addEventListener("submit", async (event) => {
  event.preventDefault();
  const parsed = Number.parseInt(compassInput.value, 10);
  const angle = Number.isInteger(parsed) ? parsed : lastAngle;
  closeCompassEditor();
  try {
    await sendAngle(angle);
  } catch (err) {
    setStatus("Turn failed");
  }
});

document.addEventListener("click", (event) => {
  if (!editingCompass) return;
  if (compassForm.contains(event.target) || compassValue.contains(event.target)) return;
  compassForm.requestSubmit();
});

function paintLidar(cm) {
  if (cm <= 10) lidarText.style.color = "red";
  else if (cm <= 30) lidarText.style.color = "orange";
  else lidarText.style.color = "white";
  lidarText.textContent = cm + " cm";
}

async function poll() {
  try {
    const response = await fetch("/getData");
    if (response.status === 401) {
      window.location.href = "/";
      return;
    }
    if (!response.ok) return;
    const data = await response.json();

    if (data.compassOk === true && Number.isInteger(data.compass)) showCompass(data.compass);
    else compassText.textContent = "--";

    if (Number.isInteger(data.lidar)) paintLidar(data.lidar);
    if (knownColors.includes(data.color)) {
      colorSwatch.style.setProperty("background-color", data.color, "important");
    }
    if (typeof data.enc === "string") encVal.textContent = data.enc;
  } catch (err) {
    console.error(err);
  }
}

window.addEventListener("beforeunload", () => {
  fetch("/logout", { keepalive: true });
});

setInterval(poll, 250);
poll();
