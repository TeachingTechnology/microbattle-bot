# Based on BurgerBot V2 by Kevin McAleer
# Modified by William Miles, 2023

from phew import logging, server, access_point, dns
from phew.template import render_template
from phew.server import redirect

from motor import Motor, pico_motor_shim
from pimoroni import REVERSED_DIR


# setup networking
DOMAIN = "HotDogBot.wireless" # Name of the HTML interface goes here
SSID = "your_ssid_here"
PASSWORD = "your_password_here"


# setup motors
motor_l = Motor(pico_motor_shim.MOTOR_1)
motor_r = Motor(pico_motor_shim.MOTOR_2)

motor_r.direction(REVERSED_DIR)

motor_l.speed(0)
motor_r.speed(0)

motor_l.enable()
motor_r.enable()


# handle joystick input from the web page
@server.route("/control", methonds=['POST'])
def control(request):
    x = request.data['x']
    y = request.data['y']
    # deadzone
    x = 0 if abs(x) < 0.2 else x
    y = 0 if abs(y) < 0.2 else y
    # simplest mapping, not very good
    motor_l.speed(x)
    motor_r.speed(y)


# handle serving the web page
@server.route("/", methods=['GET'])
def index(request):
    """ Render the Index page and respond to form requests """
    return render_template("index.html")


# handle some other networking issues that may arise
@server.route("/wrong-host-redirect", methods=["GET"])
def wrong_host_redirect(request):
  # if the client requested a resource at the wrong host then present 
  # a meta redirect so that the captive portal browser can be sent to the correct location
  body = "<!DOCTYPE html><head><meta http-equiv=\"refresh\" content=\"0;URL='http://" + DOMAIN + "'/ /></head>"
  logging.debug("body:",body)
  return body

@server.route("/hotspot-detect.html", methods=["GET"])
def hotspot(request):
    """ Redirect to the Index Page """
    return render_template("index.html")

@server.catchall()
def catch_all(request):
    """ Catch and redirect requests """
    if request.headers.get("host") != DOMAIN:
        return redirect("http://" + DOMAIN + "/wrong-host-redirect")


# final network setup and server launch
ap = access_point(SSID, PASSWORD)  # Change this to whatever Wifi SSID you wish
ip = ap.ifconfig()[0]              # Grab the IP address and store it
logging.info(f"starting DNS server on {ip}")
dns.run_catchall(ip)               # Catch all requests and reroute them
server.run()                       # Run the server
logging.info("Webserver Started")
