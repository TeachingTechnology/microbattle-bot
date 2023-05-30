# BurgerBot V2
# Kevin McAleer
# September 2022

from phew import logging, template, server, access_point, dns
from phew.template import render_template
from phew.server import redirect
from burgerbot import Burgerbot
from picozero import pico_temp_sensor, pico_led
from time import sleep
from phew import secret 
import math

mac = Burgerbot()
mac.speed = 0.5

DOMAIN = "HotDogBot.wireless" # Name of the HTML interface goes here
command = ""

@server.route("/", methods=['GET','POST'])
def index(request):
    """ Render the Index page and respond to form requests """
    if request.method == 'GET':
        logging.debug("Get request")
        return render_template("index.html")
    if request.method == 'POST':
        #print("posted content received")
        if request.form.get("forward"):
            mac.forward()
        if request.form.get("Backward"):
            mac.backward()
        if request.form.get("Stop"):
            mac.stop()
        if request.form.get("left"):
            mac.turnleft()
        if request.form.get("right"):
            mac.turnright()
        if request.form.get("Pen up"):
            mac.pen_up()
        if request.form.get("Pen down"):
            mac.pen_down()
        if request.form.get("ledoff"):
            pico_led.off()
        if request.form.get("ledon"):
            pico_led.on()
          
        
        
        command = request.form

        #print(command)
         
    return render_template("index.html")

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



ap = access_point(secret.ssid, secret.password)  # Change this to whatever Wifi SSID you wish
ip = ap.ifconfig()[0]                   # Grab the IP address and store it
logging.info(f"starting DNS server on {ip}")
dns.run_catchall(ip)                    # Catch all requests and reroute them
server.run()                            # Run the server
logging.info("Webserver Started")


