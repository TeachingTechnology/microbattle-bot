import socket
import time


with socket.socket() as connection:
	connection.bind(('localhost', 8080))
	connection.listen(1)

	last = time.time()

	while True:
		client = connection.accept()[0]
		request = client.recv(4096).decode()
		curr = time.time()
		delta = curr - last
		last = curr
		if request.startswith('GET / '):
			print("Sending index.html")
			with open('index.html', 'r') as f:
				client.send(('HTTP/1.1 200 OK\r\n\r\n' + f.read()).encode())
		elif request.startswith('GET /state/'):
			state = request.split()[1].split('/')[1:]
			client.send('HTTP/1.1 200 OK\r\n\r\n'.encode())
			print(str(delta) + ' ' + str(state))
		else:
			print(request)
			client.send('HTTP/1.1 404 Not Found\r\n\r\n'.encode())
		client.close()

