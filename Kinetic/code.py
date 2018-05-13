import time
import board
import busio
import random

from adafruit_pca9685 import PCA9685
from adafruit_motor import stepper
import math

i2c = busio.I2C(board.SCL, board.SDA)

pca = PCA9685(i2c, address=0x60)
pca.frequency = 1600
pca2 = PCA9685(i2c, address=0x61)
pca2.frequency = 1600

pcas=[pca,pca2]
channels=[7,2,8,13]

def free():
    for pca in pcas:
        for channel in channels:
            pca.channels[channel].duty_cycle = 0
    
def set_power(val):
    for pca in pcas:
        for channel in channels:
            pca.channels[channel].duty_cycle = val

#free the motors            
def free():
    set_power(0)

#power the motors    
def power():
    set_power(0xffff)

msteps=4 #number of microsteps
ll = stepper.StepperMotor(pca.channels[4], pca.channels[3],pca.channels[5], pca.channels[6],microsteps=msteps) 
lr = stepper.StepperMotor(pca.channels[10],pca.channels[9],pca.channels[11],pca.channels[12],microsteps=msteps)  
ul = stepper.StepperMotor(pca2.channels[4], pca2.channels[3],pca2.channels[5], pca2.channels[6],microsteps=msteps) 
ur = stepper.StepperMotor(pca2.channels[10],pca2.channels[9],pca2.channels[11],pca2.channels[12],microsteps=msteps)

#operation mode
#step=stepper.DOUBLE
step=stepper.SINGLE
#step=stepper.MICROSTEP
#step=stepper.INTERLEAVE

motors=[ul,ur,ll,lr]
motordir=[1,-1,1,-1] #define motor direction relative to ccw=- and cw=+ (due to wiring)

def move(motor,units): #note this routine doesn't compensate for motor wiring with motordir
    if (units<0):
        for i in range(abs(units)):
            motor.onestep(direction=stepper.BACKWARD,style=step)
    else:
        for i in range(units):
            motor.onestep(style=step)
 
seq=0
def custom_delay():
    global seq
    seq+=1
    if (seq%8<4):
        return(0.1)
    else:
        return(0.2)
    
tracks=['drum1.txt','drum2.txt']
tracknum=0
            
def move4(cur,vec):
    maxval=max([abs(max(vec)),abs(min(vec))])
    if (maxval!=0):
        stepsize=[vec[0]/maxval,vec[1]/maxval,vec[2]/maxval,vec[3]/maxval]
        c=list(cur) #copy the list to give current position of each motor
        for i in range(maxval+1):
            for j,motor in enumerate(motors):
                next=math.floor(cur[j]+i*stepsize[j]) #calculate desired int position
                if (next!=c[j]): #if not same as where we are step it
                    if (motordir[j]*vec[j]<0):
                        motor.onestep(direction=stepper.BACKWARD,style=step)
                    else:
                        motor.onestep(style=step)
                    c[j]=next #record that we stepped it
                    nextdelay(2)

def nextdelay(mult):
    global f,tracknum
    dly=60
    try:
        dly=mult*int(f.readline())/1000.0
    except:
        try:
            f.close()
        except:
            pass
        tracknum=(tracknum+1)%len(tracks)
        print("playing "+tracks[tracknum])
        f=open(tracks[tracknum])
        dly=2*int(f.readline())/1000.0
    time.sleep(dly)   

def correct():
    for i in range(4):
        ll.onestep(style=stepper.MICROSTEP)
    for i in range(2):
        ul.onestep(style=stepper.MICROSTEP)
        
shapes=[
[-25,25,25,-25,"diamond"],
[25,-25,-25,25,"cross"],
[-25,-25,25,25,"left"],
[-25,0,25,0,"arrow left"],
[25,25,-25,-25,"right"],
[0,25,0,-25,"arrow right"],
[-25,25,-25,25,"up"],
[-25,25,-50,50,"arrow up"],
[25,-25,25,-25,"down"],
[50,-50,25,-25,"arrow down"],
[50,-50,50,-50,"vert"],
[0,50,50,0,"mill1"],
[50,0,0,50,"mill2"],
[25,25,25,25,"backslash"],
[-25,-25,-25,-25,"slash"],
[0,0,0,0,"horiz"]
]

mode=3

if (mode==1):
    power()
    for i in range(4):
        for j in range(200):
            motors[i].onestep()
elif (mode==2):  
    power()
    f=open(tracks[tracknum])
    for i in range(800):
        for j in range(4):
            motors[j].onestep(style=stepper.MICROSTEP)
            motors[j].onestep(style=stepper.MICROSTEP,direction=stepper.BACKWARD)
            nextdelay(1)
    f.close()
elif (mode==3):
    power() #allow some hand calibration for 10 seconds
    time.sleep(10)
    c=[-25,25,25,-25,"diamond"] #initial position horizontal
    f=open(tracks[tracknum])
    while True:
        shape=random.choice(shapes)
        power() #power the motors
        move4(c,[shape[0]-c[0],shape[1]-c[1],shape[2]-c[2],shape[3]-c[3]]) #move to next configuration
        c=shape #update current position
        print(shape[4]) #print configuration
        free() #free motors for 5 seconds
        time.sleep(5)
    f.close()
    
pca.deinit()
pca2.deinit()
