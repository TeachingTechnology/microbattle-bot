import socket
import time


files = {
	'index.html': 'text/html',
	'index.js': 'text/javascript',
	'index.css': 'text/css'
}


with socket.socket() as connection:
	connection.bind(('localhost', 8080))
	connection.listen(1)

	last = time.time()

	while True:
		# Receive request
		client = connection.accept()[0]
		request = client.recv(4096).decode().split()
		type = request[0]
		file = request[1]

		# Calculate/update request time delta
		curr = time.time()
		delta = curr - last
		last = curr

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
			print(f"D: {delta:.3f}  X: {state[0]:+.3f}  Y: {state[1]:+.3f}  B: {state[2]}")
		else:
			print(request)
			client.send('HTTP/1.1 404 Not Found\r\n\r\n'.encode())

		client.close()

