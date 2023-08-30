import json

ssid_id = b"SSID STAND IN, FILL IN HERE"
pass_id = b"PASSWORD STAND IN, FILL IN HERE"

ssid_max = len(ssid_id)
pass_max = len(pass_id)

with open("config.json", "r") as config_file:
	config = json.load(config_file)

	ssid = config["ssid"].encode()
	password = config["password"].encode()

	if (len(ssid) > ssid_max):
		print(f"SSID is too long, max supported length is {ssid_max} characters.")
	if (len(password) > pass_max):
		print(f"Password is too long, max supported length is {pass_max} characters.")
	elif (len(password) < 8):
		print(f"Password is too short, min supported length is 8 characters.")

with open("firmware.uf2", "rb") as firmware:
	data = bytearray(firmware.read())

sloc = data.find(ssid_id)
ploc = data.find(pass_id)

data[sloc:sloc+len(ssid) + 1] = ssid + b"\0"
data[ploc:ploc+len(password) + 1] = password + b"\0"

with open("firmware_configured.uf2", "wb") as firmware:
	firmware.write(data)

