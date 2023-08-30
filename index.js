const UPDATE_PERIOD = 25; // ms
const DECIMAL_PLACES = 3;

const x_feedback = document.getElementById('x-feedback');
const y_feedback = document.getElementById('y-feedback');
const b_feedback = document.getElementById('b-feedback');

const x_value = document.getElementById('x-value');
const y_value = document.getElementById('y-value');
const b_value = document.getElementById('b-value');

const x_deadzone = document.getElementById('x-deadzone');
const y_deadzone = document.getElementById('y-deadzone');

const x_invert = document.getElementById('x-invert');
const y_invert = document.getElementById('y-invert');

const x_min = document.getElementById('x-min');
const x_max = document.getElementById('x-max');
const y_min = document.getElementById('y-min');
const y_max = document.getElementById('y-max');
const b_on  = document.getElementById('b-on');
const b_off = document.getElementById('b-off');

const controller_select = document.getElementById('controller-select');
const controller_configuration = document.getElementById('controller-configuration');

const x_axis = document.getElementById('x-axis');
const y_axis = document.getElementById('y-axis');
const button = document.getElementById('button');

const state = {
	x: 0,
	y: 0,
	b: 0
};

const keyboard = {
	l: 0,
	r: 0,
	u: 0,
	d: 0,
	b: 0
};

let gamepad;

function format_number(n) {
	return (n < 0 ? "": "+") + n.toFixed(DECIMAL_PLACES);
}

function state_to_percent(n) {
	const value = (n * 0.5 + 0.5) * 100;
	return `${value}%`;
}

function update_ui() {
	x_value.textContent = format_number(state.x);
	y_value.textContent = format_number(state.y);
	b_value.textContent = state.b;
	x_feedback.style.left = state_to_percent(state.x);
	y_feedback.style.left = state_to_percent(state.y);
	b_feedback.style['background-color'] = state.b == b_on.valueAsNumber ? "black" : "lightgrey";
}

function apply_state_constraints() {
	state.x = state.x < x_max.valueAsNumber ? state.x : x_max.valueAsNumber;
	state.y = state.y < y_max.valueAsNumber ? state.y : y_max.valueAsNumber;
	state.x = state.x > x_min.valueAsNumber ? state.x : x_min.valueAsNumber;
	state.y = state.y > y_min.valueAsNumber ? state.y : y_min.valueAsNumber;
	state.x *= x_invert.checked ? -1 : 1;
	state.y *= y_invert.checked ? -1 : 1;
	state.b = state.b ? b_on.valueAsNumber: b_off.valueAsNumber;
}

function update_state_from_keyboard() {
	state.x = keyboard.r - keyboard.l;
	state.y = keyboard.d - keyboard.u;
	state.b = keyboard.b;
	apply_state_constraints();
	update_ui();
}

// handle a key being pressed/released, value should be 0 for
// key up and 1 for key down.
function key_change(e, value) {
	// always update the saved keyboard state
	switch (e.code) {
		case "KeyA":
		case "ArrowLeft":  keyboard.l = value; break;
		case "KeyD":
		case "ArrowRight": keyboard.r = value; break;
		case "KeyS":
		case "ArrowDown":  keyboard.d = value; break;
		case "KeyW":
		case "ArrowUp":    keyboard.u = value; break;
		case "Space":      keyboard.b = value; break;
	}
	// only change state if the keyboard is the current controller
	if (!gamepad) {
		update_state_from_keyboard();
	}
}
const key_down = (e) => key_change(e, 1);
const key_up   = (e) => key_change(e, 0);

// handle gamepad connected/disconnected events
function gamepad_connection(e) {
	// had to delay this as the array returned from getGamepads wasn't
	// being updated fast enough when the controller was disconnected
	setTimeout(() =>
		{
			const gamepads = navigator.getGamepads();
			const prev_selected = controller_select.value;

			// remove all current gamepad options (not keyboard)
			while (controller_select.children.length !== 1) {
				controller_select.removeChild(controller_select.children[1]);
			}

			// re-add gamepad options from the currently available
			// gamepads
			for (let i = 0; i !== gamepads.length; ++i) {
				const option = document.createElement('option');
				option.value = gamepads[i].index;
				option.text = `${gamepads[i].id} (${gamepads[i].index})`;

				controller_select.appendChild(option);
			}

			// try to keep current controller selection, otherwise
			// default to the keyboard
			controller_select.value = prev_selected;
			if (!controller_select.value) {
				controller_select.value = 'keyboard';
			}

			// manually trigger selection event
			controller_select.dispatchEvent(new Event('change'));
		},
		100
	);
}

// get gamepad by its unique index, index must be a Number
function get_gamepad_by_index(index) {
	const gamepads = navigator.getGamepads();
	for (let i = 0; i !== gamepads.length; ++i) {
		if (gamepads[i].index === index) return gamepads[i];
	}
	return undefined;
}

// handle a new controller being selected
function gamepad_select(event) {
	if (controller_select.value === 'keyboard') {
		gamepad = undefined;
		controller_configuration.style.display = "none";
		update_state_from_keyboard();
	} else {
		gamepad = get_gamepad_by_index(parseInt(controller_select.value));
		controller_configuration.style.display = "block";
		update_select(x_axis, gamepad.axes);
		update_select(y_axis, gamepad.axes);
		update_select(button, gamepad.buttons);
	}
	// make the select element not active so that keyboard input
	// doesn't accidentally change the controller
	document.activeElement.blur();
}

function update_select(select, array) {
	const prev_value = select.value;

	while (select.children.length !== 0) {
		select.removeChild(select.children[0]);
	}

	for (let i = 0; i !== array.length; ++i) {
		const option = document.createElement('option');
		option.value = i;
		option.text = i;
		select.appendChild(option);
	}

	select.value = prev_value === "" ? 0 : prev_value;
}

// handle input from the gamepad
// immediately invoked to start update callbacks
(function gamepad_update() {
	if (gamepad !== undefined) {
		gamepad = get_gamepad_by_index(parseInt(controller_select.value));
		let x = gamepad.axes[x_axis.value];
		let y = gamepad.axes[y_axis.value];
		// apply deadzone
		x = Math.abs(x) >= x_deadzone.valueAsNumber ? x : 0;
		y = Math.abs(y) >= y_deadzone.valueAsNumber ? y : 0;
		// update state and UI
		state.x = x;
		state.y = y;
		state.b = gamepad.buttons[button.value].value;
		apply_state_constraints();
		update_ui();
	}

	requestAnimationFrame(gamepad_update);
})();

// register required listeners
window.addEventListener('keydown', key_down);
window.addEventListener('keyup', key_up);
window.addEventListener('gamepadconnected', gamepad_connection);
window.addEventListener('gamepaddisconnected', gamepad_connection);

// register and fire controller selection event for startup
controller_select.addEventListener('change', gamepad_select);
controller_select.dispatchEvent(new Event('change'));

// used to drop responses from the fetch that's being used to send
// state to the robot
function noop() {}

while (true) {
	// send state to the robot
	fetch(`${document.baseURI}state/${state.x}/${state.y}/${state.b}`).then(noop, noop);
	// wait the alloted update period (so we don't spam the robot)
	await new Promise(r => setTimeout(r, UPDATE_PERIOD));
}
