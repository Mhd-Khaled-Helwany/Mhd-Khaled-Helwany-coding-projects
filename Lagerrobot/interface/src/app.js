const API_BASE = "http://10.42.0.1:5001";

const CONTROL_ROUTES = {
	forward: { path: "/move", payload: { direction: 0 }, method: "POST" },
	backward: { path: "/move", payload: { direction: 1 }, method: "POST" },
	left: { path: "/turn", payload: { direction: 0 }, method: "POST" },
	right: { path: "/turn", payload: { direction: 1 }, method: "POST" },
	stop: { path: "/stop", method: "POST" },
	switchAuto: { path: "/switchAuto", method: "POST" },
	pickupLeft: { path: "/startPickup", payload: { direction: "left" }, method: "POST" },
	pickupRight: { path: "/startPickup", payload: { direction: "right" }, method: "POST" },
	endPickup: { path: "/endPickup", method: "POST" },
	dropoff: { path: "/dropoff", method: "POST" },
	resetGyro: { path: "/resetGyro", method: "POST" },
	test: { path: "/test", method: "GET" },
	openClaw: { path: "/openClaw", method: "POST" },
	closeClaw: { path: "/closeClaw", method: "POST" },
};

const ARM_ROUTES = {
	armRotation: { path: "/turnArmRotation", method: "POST" },
	topAxle: { path: "/turnTopAxle", method: "POST" },
	middleAxle: { path: "/turnMiddleAxle", method: "POST" },
	baseAxle: { path: "/turnBaseAxle", method: "POST" },
	clawRotation: { path: "/clawRotationAxle", method: "POST" },
};

const STATUS_ROUTES = {
	ultrasound: { path: "/getUltrasound", method: "GET" },
	gyro: { path: "/getGyroAngle", method: "GET" },
	rawGyro: { path: "/getRawGyro", method: "GET" },
	sensorTest: { path: "/getSensorTest", method: "GET" },
	getMode: { path: "/getMode", method: "GET" }, // Returns "auto" or "manual"
	getAutoStatus: { path: "/getAutoStatus", method: "GET" },
	getAutoPositioning: { path: "/getAutoPositioning", method: "GET" },
	test: { path: "/test", method: "GET" },
	lineSensors: { path: "/getLineSensor", method: "GET" },
	proportionalParemeter: { path: "/getProportionalParameter", method: "GET" },
	angleParameter: { path: "/getAngleParameter", method: "GET" },
	connection: { path: "/test", method: "GET" }, // Connection check
	steerSignal: { path: "/getSteerSignal", method: "GET" },
	turnSpeed: { path: "/getTurnSpeed", method: "GET" },
	driveSpeed: { path: "/getDriveSpeed", method: "GET" },
};

let isAutonomous = false;
let pollEnabled = true;
const SENSOR_POLL_SEQUENCE = [
	{ name: "connection", route: STATUS_ROUTES.connection, apply: updateConnection },
	{ name: "ultrasound", route: STATUS_ROUTES.ultrasound, apply: updateUltrasound },
	{ name: "gyro-angle", route: STATUS_ROUTES.gyro, apply: updateGyroAngle },
	{ name: "gyro-raw", route: STATUS_ROUTES.rawGyro, apply: updateGyroRaw },
	{ name: "auto-status", route: STATUS_ROUTES.getAutoStatus, apply: updateAutoStatus },
	{ name: "line-sensors", route: STATUS_ROUTES.lineSensors, apply: updateLineSensors },
	{ name: "proportional-parameter", route: STATUS_ROUTES.proportionalParemeter, apply: updateProportionalParameter },
	{ name: "angle-parameter", route: STATUS_ROUTES.angleParameter, apply: updateAngleParameter },
	{ name: "get-auto-positioning", route: STATUS_ROUTES.getAutoPositioning, apply: updateAutoPositioning },
	{ name: "steer-signal", route: STATUS_ROUTES.steerSignal, apply: updateSteerSignal },
	{ name: "turn-speed", route: STATUS_ROUTES.turnSpeed, apply: updateTurnSpeed },
	{ name: "drive-speed", route: STATUS_ROUTES.driveSpeed, apply: updateDriveSpeed }
];
const MIN_SENSOR_DELAY_MS = 100;
let lastSensorRequestAt = 0;
const KEYMAP = {
	ArrowUp: "forward",
	ArrowDown: "backward",
	ArrowLeft: "left",
	ArrowRight: "right",
	" ": "stop",
};
const DIRECTION_KEYS = new Set(["ArrowUp", "ArrowDown", "ArrowLeft", "ArrowRight"]);
let heldDirectionalKey = null;
const LINE_SENSOR_ROWS = [
	{ key: "rowA", label: "Sensor Row A" },
	{ key: "rowB", label: "Sensor Row B" },
];
const LINE_SENSOR_COUNT = 11;
const LINE_SENSOR_MAX_VALUE = 1023;
const lineSensorElements = {
	miniContainer: null,
	miniRows: {},
};

async function sendCommand(control) {
	const route = CONTROL_ROUTES[control];
	if (!route) return;

	const options = {
		method: route.method,
		headers: {
			"Content-Type": "application/json",
		},
		body: route.payload ? JSON.stringify(route.payload) : undefined,
	};

	try {
		console.log(`Sending ${control} request to ${route.path}...`);
		await fetch(`${API_BASE}${route.path}`, options);
		console.log(`Request sent: ${control} (${route.method})`);
	} catch (err) {
		console.warn(`CORS block detected for ${control}, retrying without CORS`, err);
		console.log(`CORS blocked ${control}, retrying without CORS...`);
		try {
			await fetch(`${API_BASE}${route.path}`, { ...options, mode: "no-cors" });
			console.log(`Request sent without CORS for ${control}`);
		} catch (secondaryError) {
			console.error(`Failed to send ${control} command`, secondaryError);
			console.log(`Failed to send ${control}: ${secondaryError.message}`);
		}
	}
}

async function sendArmCommand(armControl, diffAngle) {
	const route = ARM_ROUTES[armControl];
	if (!route) return;

	const options = {
		method: route.method,
		headers: {
			"Content-Type": "application/json",
		},
		body: JSON.stringify({ diff_angle: diffAngle }),
	};

	try {
		await fetch(`${API_BASE}${route.path}`, options);
	} catch (err) {
		try {
			await fetch(`${API_BASE}${route.path}`, { ...options, mode: "no-cors" });
		} catch (secondaryError) {
			console.error(`Failed to send arm command ${armControl}`, secondaryError);
		}
	}
}

function cancelKeyHold() {
	heldDirectionalKey = null;
}

function updateText(selector, value) {
	const el = document.querySelector(selector);
	if (el) el.textContent = value;
}

function updateConnection(data) {
	const connectionElement = document.querySelector("[data-connection-status]");
	if (!connectionElement) return;

	// If we get any response, we're connected
	if (data !== null && data !== undefined) {
		connectionElement.textContent = "Connected";
		connectionElement.className = "stat__value pill pill--ok";
	} else {
		connectionElement.textContent = "Disconnected";
		connectionElement.className = "stat__value pill pill--error";
	}
}

function updateUltrasound(data) {
	const distance = data?.distance_cm ?? data?.distance ?? data;
	if (distance === undefined || distance === null) return;
	if (distance === 689) return;
	updateText("[data-ultrasound]", `${distance} cm`);
}

function updateGyroAngle(data) {
	const angleUnits = data?.angle ?? data;
	if (angleUnits === undefined || angleUnits === null) return;
	const formatted = `${angleUnits} °`;
	updateText("[data-gyro-angle]", formatted);
}

function updateGyroRaw(data) {
	if (data === undefined || data === null) return;
	if (typeof data === "object") {
		updateText("[data-gyro-raw]", JSON.stringify(data));
		return;
	}
	updateText("[data-gyro-raw]", String(data));
}

function updateProportionalParameter(data) {
	const param = data?.proportional ?? data;
	if (param === undefined || param === null) return;
	updateText("[data-proportional-parameter]", String(param));
}

function updateAngleParameter(data) {
	const param = data?.angle ?? data;
	if (param === undefined || param === null) return;
	updateText("[data-angle-parameter]", String(param));
}

function updateAutoPositioning(data) {
	const current_position = data?.current_position;
	const destination_position = data?.destination_position;
	const in_factory = data?.in_factory;
	if (current_position === undefined || current_position === null) return;
	updateText("[data-current-position]", String(current_position));
	if (destination_position === undefined || destination_position === null) return;
	updateText("[data-destination-position]", String(destination_position));
	if (in_factory === undefined || in_factory === null) return;
	updateText("[data-in-factory]", String(in_factory));
}

function updateSteerSignal(data) {
	const steer_signal = data;
	if (steer_signal === undefined || steer_signal === null) return;
	updateText("[data-steer-signal]", String(steer_signal).slice(0, 4));
}

function updateAutoStatus(data) {
	const status = data?.auto_status ?? data;
	if (status === undefined || status === null) return;
	updateText("[data-auto-status]", String(status));
}

function updateTurnSpeed(data) {
	const turnSpeed = data?.speed;
	if (turnSpeed === undefined || turnSpeed === null) return;
	updateText("[data-turn-speed]", String(turnSpeed))
}

function updateDriveSpeed(data) {
	const driveSpeed = data?.speed;
	if (driveSpeed === undefined || driveSpeed === null) return;
	updateText("[data-drive-speed]", String(driveSpeed))
}

function clampLineSensorValue(value) {
	if (value === null || value === undefined) return null;
	const numeric = Number(value);
	if (!Number.isFinite(numeric)) return null;
	const clamped = Math.min(Math.max(numeric, 0), LINE_SENSOR_MAX_VALUE);
	return Math.round(clamped);
}

function normalizeLineSensorRow(values) {
	if (!Array.isArray(values) || values.length !== LINE_SENSOR_COUNT) return null;
	const normalized = [];
	for (let i = 0; i < LINE_SENSOR_COUNT; i += 1) {
		const nextValue = clampLineSensorValue(values[i]);
		normalized.push(nextValue);
	}
	return normalized;
}

function normalizeLineSensorPayload(payload) {
	if (!payload || typeof payload !== "object") return null;

	const source = payload.tape_sensor_data ?? payload;
	if (!source || typeof source !== "object") return null;

	const frontValues = normalizeLineSensorRow(
		source.front_sensor ?? source.frontSensor ?? source.front
	);
	const rearValues = normalizeLineSensorRow(
		source.rear_sensor ?? source.rearSensor ?? source.rear ?? source.back
	);

	if (!frontValues || !rearValues) return null;

	// Map front/rear readings onto the two UI rows.
	return {
		rowA: frontValues,
		rowB: rearValues,
	};
}

function updateMiniLineSensors(rowKey, values) {
	const miniRow = lineSensorElements.miniRows[rowKey];
	if (!miniRow) return;

	const bars = miniRow.querySelectorAll(".mini-sensor-bar");

	bars.forEach((bar, index) => {
		const reading = Array.isArray(values) ? values[index] : null;

		if (reading === null || reading === undefined) {
			bar.style.backgroundColor = "#ef4444";
			return;
		}

		const clamped = clampLineSensorValue(reading);
		if (clamped === null) {
			bar.style.backgroundColor = "#ef4444";
			return;
		}

		// Calculate grayscale: 0 = white (#ffffff), 1023 = black (#000000)
		const intensity = Math.round((clamped / LINE_SENSOR_MAX_VALUE) * 255);
		const hexValue = (255 - intensity).toString(16).padStart(2, '0');
		const color = `#${hexValue}${hexValue}${hexValue}`;
		bar.style.backgroundColor = color;
	});
}

function updateLineSensors(data) {
	const normalized = normalizeLineSensorPayload(data);

	LINE_SENSOR_ROWS.forEach((rowDef) => {
		const values = normalized ? normalized[rowDef.key] : null;
		updateMiniLineSensors(rowDef.key, values);
	});
}

function applyPollUI(isEnabled) {
	const toggle = document.querySelector("[data-poll-toggle]");
	if (!toggle) return;

	toggle.classList.toggle("is-active", isEnabled);
	toggle.setAttribute("aria-pressed", String(isEnabled));
}

async function fetchModeStatus() {
	const route = STATUS_ROUTES.getMode;
	if (!route) return null;

	try {
		console.log(`Fetching driving mode from ${route.path}...`);
		const response = await fetch(`${API_BASE}${route.path}`, { method: route.method });
		if (!response.ok) return null;

		const modeText = (await response.text()).trim().toLowerCase();
		if (modeText.includes("auto")) {
			console.log("Mode reported: autonomous");
			return "auto";
		}
		if (modeText.includes("manual")) {
			console.log("Mode reported: manual");
			return "manual";
		}
	} catch (err) {
		console.warn("Unable to read current driving mode", err);
		console.log("Unable to read current driving mode");
	}

	return null;
}

function applyModeUI(isAuto) {
	isAutonomous = isAuto;

	const manualControls = document.querySelector("[data-manual-controls]");
	const autoOverlay = document.querySelector("[data-auto-overlay]");
	const modeToggle = document.querySelector("[data-auto-toggle]");
	const modeIndicator = document.querySelector("[data-mode-indicator]");
	const autoOnly = document.querySelectorAll("[data-auto-only]");

	if (manualControls) manualControls.classList.toggle("is-hidden", isAuto);
	if (autoOverlay) autoOverlay.hidden = !isAuto;

	autoOnly.forEach(element => {
		element.hidden = !isAuto;
	});

	if (modeToggle) {
		modeToggle.classList.toggle("is-active", isAuto);
		modeToggle.setAttribute("aria-pressed", String(isAuto));
	}

	const modeLabel = isAuto ? "Autonomous" : "Manual";
	if (modeIndicator) modeIndicator.textContent = modeLabel;


	if (isAuto && heldDirectionalKey) {
		cancelKeyHold();
		sendCommand("stop");
	}
}

function delay(ms) {
	return new Promise((resolve) => setTimeout(resolve, ms));
}

async function pollSensor(routeDef, applyFn) {
	try {
		const response = await fetch(`${API_BASE}${routeDef.path}`, { method: routeDef.method });
		if (!response.ok) return;

		let payload = null;
		const contentType = response.headers.get("Content-Type") || "";
		if (contentType.includes("application/json")) {
			payload = await response.json();
		} else {
			const text = (await response.text()).trim();
			try {
				payload = text ? JSON.parse(text) : text;
			} catch {
				payload = text;
			}
		}

		applyFn(payload);
	} catch (err) {
		console.log(`Sensor read failed for ${routeDef.path}`);
	}
}

async function startSensorPolling() {
	while (true) {
		if (!pollEnabled) {
			await delay(200);
			continue;
		}

		for (const item of SENSOR_POLL_SEQUENCE) {
			const now = performance.now();
			const elapsed = now - lastSensorRequestAt;
			if (elapsed < MIN_SENSOR_DELAY_MS) {
				await delay(MIN_SENSOR_DELAY_MS - elapsed);
			}
			lastSensorRequestAt = performance.now();
			await pollSensor(item.route, item.apply);
		}
	}
}

function handleKeydown(event) {
	if (isAutonomous) return;

	const control = KEYMAP[event.key];
	if (!control) return;

	// Instant stop button
	if (event.key === " ") {
		event.preventDefault();
		cancelKeyHold();
		sendCommand("stop");
		return;
	}

	if (!DIRECTION_KEYS.has(event.key)) return;
	event.preventDefault();

	if (heldDirectionalKey === event.key) return;

	heldDirectionalKey = event.key;
	sendCommand(control);
}

function handleKeyup(event) {
	if (isAutonomous) return;
	if (!DIRECTION_KEYS.has(event.key)) return;
	if (heldDirectionalKey && heldDirectionalKey !== event.key) return;

	event.preventDefault();
	cancelKeyHold();
	sendCommand("stop");
}

function initKeyboardControls() {
	window.addEventListener("keydown", handleKeydown);
	window.addEventListener("keyup", handleKeyup);
	window.addEventListener("blur", () => {
		const hadHeld = Boolean(heldDirectionalKey);
		cancelKeyHold();
		if (hadHeld && !isAutonomous) {
			sendCommand("stop");
		}
	});
}

const ARM_STEP = 5;
const ARM_INTERVAL_MS = 200;
let activeArmIntervals = new Map();

function startArmControl(armControl, diffAngle) {
	if (activeArmIntervals.has(armControl)) return;

	sendArmCommand(armControl, diffAngle);

	const intervalId = setInterval(() => {
		sendArmCommand(armControl, diffAngle);
	}, ARM_INTERVAL_MS);

	activeArmIntervals.set(armControl, intervalId);
}

function stopArmControl(armControl) {
	const intervalId = activeArmIntervals.get(armControl);
	if (intervalId) {
		clearInterval(intervalId);
		activeArmIntervals.delete(armControl);
	}
}

function stopAllArmControls() {
	activeArmIntervals.forEach((intervalId) => clearInterval(intervalId));
	activeArmIntervals.clear();
}

function initArmControls() {
	document.querySelectorAll("[data-arm-control]").forEach((button) => {
		const armControl = button.dataset.armControl;
		const direction = button.dataset.armDirection;
		const diffAngle = direction === "positive" ? ARM_STEP : -ARM_STEP;

		const startHandler = (e) => {
			e.preventDefault();
			startArmControl(armControl, diffAngle);
		};
		const stopHandler = () => stopArmControl(armControl);

		button.addEventListener("mousedown", startHandler);
		button.addEventListener("mouseup", stopHandler);
		button.addEventListener("mouseleave", stopHandler);
		button.addEventListener("touchstart", startHandler);
		button.addEventListener("touchend", stopHandler);
	});

	window.addEventListener("blur", stopAllArmControls);
}

document.addEventListener("DOMContentLoaded", () => {
	const modeToggle = document.querySelector("[data-auto-toggle]");

	document.querySelectorAll("[data-control]").forEach((button) =>
		button.addEventListener("click", () => sendCommand(button.dataset.control))
	);

	initKeyboardControls();
	initArmControls();

	// Initialize mini line sensor references
	lineSensorElements.miniContainer = document.querySelector("[data-mini-line-sensors]");
	if (lineSensorElements.miniContainer) {
		LINE_SENSOR_ROWS.forEach((rowDef) => {
			const miniRow = lineSensorElements.miniContainer.querySelector(`[data-mini-row="${rowDef.key.charAt(3)}"]`);
			if (miniRow) {
				lineSensorElements.miniRows[rowDef.key] = miniRow;
			}
		});
	}

	if (modeToggle) {
		modeToggle.addEventListener("click", async () => {
			const nextModeIsAuto = !isAutonomous;
			await sendCommand("switchAuto");
			await sendCommand("resetGyro");
			applyModeUI(nextModeIsAuto);
		});
	}

	const pollToggle = document.querySelector("[data-poll-toggle]");
	if (pollToggle) {
		pollToggle.addEventListener("click", () => {
			pollEnabled = !pollEnabled;
			console.log(pollEnabled ? "Live refresh enabled" : "Live refresh paused");
			applyPollUI(pollEnabled);
		});
		applyPollUI(pollEnabled);
	}

	fetchModeStatus().then((mode) => {
		if (mode) applyModeUI(mode === "auto");
	});

	sendCommand("resetGyro");
	startSensorPolling();

	// Control parameters form logic
	const paramsForm = document.querySelector(".params-form");
	if (paramsForm) {
		paramsForm.addEventListener("submit", async (e) => {
			e.preventDefault();
			const proportionalInput = paramsForm.querySelector("input[name='proportional']");
			const angleInput = paramsForm.querySelector("input[name='angle']");
			const proportional = Number(proportionalInput.value);
			const angle = Number(angleInput.value);

			try {
				console.log(`Updating control parameters: proportional=${proportional}, angle=${angle}`);
				const response = await fetch(`${API_BASE}/updateControl`, {
					method: "POST",
					headers: { "Content-Type": "application/json" },
					body: JSON.stringify({ proportional, angle })
				});
				const feedbackEl = document.getElementById('param-feedback');
				if (response.ok) {
					console.log("Control parameters updated successfully.");
					feedbackEl.textContent = "Parameters updated successfully";
					feedbackEl.className = "param-feedback success";
					setTimeout(() => {
						feedbackEl.className = "param-feedback";
					}, 3000);
				} else {
					console.log("Failed to update control parameters.");
					feedbackEl.textContent = "Failed to update parameters";
					feedbackEl.className = "param-feedback error";
					setTimeout(() => {
						feedbackEl.className = "param-feedback";
					}, 3000);
				}
			} catch (err) {
				console.log("Error updating control parameters: " + err.message);
				const feedbackEl = document.getElementById('param-feedback');
				feedbackEl.textContent = "Error updating parameters";
				feedbackEl.className = "param-feedback error";
				setTimeout(() => {
					feedbackEl.className = "param-feedback";
				}, 3000);
			}
		});
	}

	const speedForm = document.querySelector(".speed-form");
	if (speedForm) {
		speedForm.addEventListener("submit", async (e) => {
			e.preventDefault();
			const turnSpeedInput = speedForm.querySelector("input[name='turnSpeed']");
			const driveSpeedInput = speedForm.querySelector("input[name='driveSpeed']");
			const turnSpeed = Number(turnSpeedInput.value);
			const driveSpeed = Number(driveSpeedInput.value);

			try {
				const response = await fetch(`${API_BASE}/setSpeed`, {
					method: "POST",
					headers: { "Content-Type": "application/json" },
					body: JSON.stringify({ turnSpeed, driveSpeed })
				});
			}
			catch (err) {

			}
		});
	}
});
