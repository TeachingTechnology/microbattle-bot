import socket
import struct

files = {
	'index.html': 'text/html',
	'index.js': 'text/javascript',
	'index.css': 'text/css'
}

PICO_IP = "192.168.0.72"
PICO_PORT = 12345

with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as to_pico:
	with socket.socket() as local:
		local.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
		local.bind(('localhost', 8080))
		local.listen(1)

		while True:
			# Receive request
			(client, address) = local.accept()
			request = client.recv(4096).decode().split()
			type = request[0]
			file = request[1]

			# Redirect root requests
			if type == 'GET' and file == '/':
				file = '/index.html'

			# Handle request
			path = file.split('/')[1:]
			if type == 'GET' and path[0] in files:
				print(f"Sending {path[0]}")
				with open(path[0], 'r') as f:
					client.send(f"HTTP/1.1 200 OK\r\nContent-Type: {files[path[0]]}; charset=utf-8\r\n\r\n{f.read()}".encode())
			elif type == 'GET' and path[0] == 'state':
				state = [float(x) for x in path[1:]]
				client.send('HTTP/1.1 200 OK\r\n\r\n'.encode())
				print(f"X: {state[0]:+.3f}  Y: {state[1]:+.3f}  B: {state[2]}")
				payload = struct.pack("<fff", state[0], state[1], state[2])
				to_pico.sendto(payload, (PICO_IP, PICO_PORT))
			else:
				print(request)
				client.send('HTTP/1.1 404 Not Found\r\n\r\n'.encode())

			client.close()

