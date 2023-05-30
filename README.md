# microbattle-bot

this project aims to use a Pico W as a wifi host for a plastic antweight battlebot.
using HTML gamepad api as input for PWM output.

what we have-

code that runs a hotspot accissible only vie correct access code. 
code that can output specific PWM to a servo. 

what we want-

interpertation of gamepad API as a data format
comprehention of that data to corespond to a specific pwm position
  this shoudl work somehwat like a joysitck for an RC car. 

Data flow - 
index hosts webpage that generates post requests
gamepad movements generate post request values
values are assigned from specific gamepad variable to specific PWM pin output

~sudo~ 
gamepad val = +1 ~~ servo 1 = 90deg
