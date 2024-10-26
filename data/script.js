// Rotation speed display
var rotation_speed = document.getElementById("rotation_speed");
var rotation_speed_disp = document.getElementById("rotation_speed_disp");
rotation_speed_disp.innerHTML = rotation_speed.value + "%";

rotation_speed.oninput = function() {
    rotation_speed_disp.innerHTML = this.value + "%";
    updateValues();
}

// Interval time display
var interval_time = document.getElementById("interval_time");
var interval_time_disp = document.getElementById("interval_time_disp");
interval_time_disp.innerHTML = interval_time.value + "s";

interval_time.oninput = function() {
    interval_time_disp.innerHTML = this.value + "s";
    updateValues();
}

// Fetch current settings from the server on page load
window.onload = function() {
    fetch("/get_settings", {
        method: "GET"
    }).then(response => response.json()).then(data => {
        rotation_speed.value = data.rotation_speed;
        rotation_speed_disp.innerHTML = data.rotation_speed + "%";
        interval_time.value = data.interval_time;
        interval_time_disp.innerHTML = data.interval_time + "s";
    }).catch(error => {
        console.error("Failed to load settings", error);
    });
}

function updateValues() {
    let rotationSpeedValue = rotation_speed.value;
    let intervalTimeValue = interval_time.value;

    fetch("/update", {
        method: "POST",
        headers: {
            "Content-Type": "application/x-www-form-urlencoded"
        },
        body: `rotation_speed=${rotationSpeedValue}&interval_time=${intervalTimeValue}`
    }).then(response => {
        if (response.ok) {
            console.log("Values updated");
        } else {
            console.error("Failed to update values");
        }
    });
}

// Start button click event
document.getElementById("start_button").onclick = function() {
    fetch("/start", {
        method: "POST"
    }).then(response => {
        if (response.ok) {
            console.log("Motor started");
        } else {
            console.error("Failed to start motor");
        }
    });
}

// Stop button click event
document.getElementById("stop_button").onclick = function() {
    fetch("/stop", {
        method: "POST"
    }).then(response => {
        if (response.ok) {
            console.log("Motor stopped");
        } else {
            console.error("Failed to stop motor");
        }
    });
}

// Save button click event
document.getElementById("save_button").onclick = function() {
    let rotationSpeedValue = rotation_speed.value;
    let intervalTimeValue = interval_time.value;

    fetch("/save", {
        method: "POST",
        headers: {
            "Content-Type": "application/x-www-form-urlencoded"
        },
        body: `rotation_speed=${rotationSpeedValue}&interval_time=${intervalTimeValue}`
    }).then(response => {
        if (response.ok) {
            console.log("Settings saved");
        } else {
            console.error("Failed to save settings");
        }
    });
}
