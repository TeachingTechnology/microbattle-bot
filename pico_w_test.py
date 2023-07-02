import network
import socket
from time import sleep
from picozero import PWMOutputDevice
import machine

ssid = 'pico_w_test_ap'
password = 'Th1s is & passw0rd'

d0 = PWMOutputDevice(21)
d1 = PWMOutputDevice(22)

def connect():
    # Conncet to WLAN
    ap = network.WLAN(network.AP_IF)
    ap.config(essid=ssid, password=password)
    ap.active(True)
    ip = ap.ifconfig()[0]
    print(f'Set up on {ip}')
    return ip

def open_socket(ip):
    # Open a socket
    address = (ip, 80)
    connection = socket.socket()
    connection.bind(address)
    connection.listen(1)
    return connection

def webpage():
    # Template HTML
    html = """
<!DOCTYPE html>
<html>
    <body>
		<script type="module">
			const LEFT  = 37;
			const UP    = 38;
			const RIGHT = 39;
			const DOWN  = 40;

			let state = [0.5, 0.5];

			function keydown(e) {
				switch (e.keyCode) {
					case LEFT:  state[0] -= 0.5; break;
					case RIGHT: state[0] += 0.5; break;
					case DOWN:  state[1] -= 0.5; break;
					case UP:    state[1] += 0.5; break;
				}
			}

			function keyup(e) {
				switch (e.keyCode) {
					case LEFT:  state[0] += 0.5; break;
					case RIGHT: state[0] -= 0.5; break;
					case DOWN:  state[1] += 0.5; break;
					case UP:    state[1] -= 0.5; break;
				}
			}

			document.addEventListener('keydown', keydown);
			document.addEventListener('keyup', keyup);

			function noop() {}

			while (true) {
				fetch(`${document.baseURI}/${state[0]}/${state[1]}`).then(noop, noop);
				await new Promise(r => setTimeout(r, 100));
			}

		</script>
    </body>
</html>

    """
    return str(html)

def serve(connection):
    # Start a web server
    state = 'OFF'
    pico_led.off()
    temperature = 0
    while True:
        client = connection.accept()[0]
        request = client.recv(1024)
        request = str(request)
        try:
            request = request.split()[1]
        except IndexError:
            pass
        if request == '/lighton?':
            pico_led.on()
            state = 'ON'
        elif request == '/lightoff?':
            pico_led.off()
            state = 'OFF'
        temperature = pico_temp_sensor.temp
        html = webpage(temperature, state)
        client.send(html)
        client.close()
        
try:
    ip = connect()
    connection = open_socket(ip)
    serve(connection)
except KeyboardInterrupt:
    machine.reset()    
