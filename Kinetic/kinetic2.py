from Tkinter import *
import math
import time
from segment import Segment

master=Tk()
s=4
w=23.75*s*10.0 #set the width of our canvas
h=23.75*s*10.0 #set the hieght of our canvas
white=(255,255,255)
canvas=Canvas(master,width=w,height=h)
canvas.pack()
mode=1
modes=[
    [1,-1,0.5,-0.5,90],
    [1,-1,1,-1,45],
    [1,1,-1,-1,45],
    [-1,-1,1,1,45],
    [1,1,-0.5,-0.5,45]
]

topx=w/2
rt2=math.sqrt(2)
diag=rt2*120.0*s
leg=5*11.5*s/rt2
yleg=leg+2.5*s
topy=(h-diag)/2.0
lx=topx-leg
ty=topy+yleg
rx=topx+leg
by=topy+diag-yleg-2.5*s

def get64(x):
    frac=round((x-int(x))*64)
    return("%d %d/64"%(int(x),frac))

print("Canvas = %f x %f"%(w/s/10.0,h/s/10.0))
print("Upper Left Center = (%s,%s) = (%f,%f)"%(get64(lx/s/10.0),get64(ty/s/10.0),lx/s/10.0,ty/s/10.0))
print("Upper Left Center = (-%s,-%s) = (-%f,-%f)"%(get64(23.75-lx/s/10.0),get64(23.75-ty/s/10.0),23.75-lx/s/10.0,23.75-ty/s/10.0))
print("Upper Right Center = (%s,%s) = (%f,%f)"%(get64(rx/s/10.0),get64(ty/s/10.0),rx/s/10.0,ty/s/10.0))
print("Upper Right Center = (-%s,-%s) = (-%f,-%f)"%(get64(23.75-rx/s/10.0),get64(23.75-ty/s/10.0),23.75-rx/s/10.0,23.75-ty/s/10.0))
print("Lower Left Center = (%s,%s) = (%f,%f)"%(get64(lx/s/10.0),get64(by/s/10.0),lx/s/10.0,by/s/10.0))
print("Lower Left Center = (-%s,-%s) = (-%f,-%f)"%(get64(23.75-lx/s/10.0),get64(23.75-by/s/10.0),23.75-lx/s/10.0,23.75-by/s/10.0))
print("Lower Right Center = (%s,%s) = (%f,%f)"%(get64(rx/s/10.0),get64(by/s/10.0),rx/s/10.0,by/s/10.0))
print("Lower Right Center = (-%s,-%s) = (-%f,-%f)"%(get64(23.75-rx/s/10.0),get64(23.75-by/s/10.0),23.75-rx/s/10.0,23.75-by/s/10.0))

#bar are 12 inches by 0.5 inch (scale=10)
sl=120*s
sw=5*s

lines=[
    Segment(canvas,lx,ty,sl,sw,-45), #tl
    Segment(canvas,rx,ty,sl,sw,45), #tr
    Segment(canvas,lx,by,sl,sw,45),#bl
    Segment(canvas,rx,by,sl,sw,-45) #br
]

def angle():
    return(angle)

def draw():
    for line in lines:
        line.draw()
    canvas.update()

def animate():
    global a,canvas
    m=25*s
    while True: #forever
        canvas.delete("all")
        #canvas.create_line(150*s-m,150*s-m,350*s+m,150*s-m,fill="red") #top
        #canvas.create_line(150*s-m,350*s+m,350*s+m,350*s+m,fill="red") #bottom
        #canvas.create_line(150*s-m,150*s-m,150*s-m,350*s+m,fill="red") #left
        #canvas.create_line(350*s+m,150*s-m,350*s+m,350*s+m,fill="red") #right
        lines[0].update(modes[mode][0])
        lines[1].update(modes[mode][1])
        lines[2].update(modes[mode][2])
        lines[3].update(modes[mode][3])
        draw()
        if (lines[0].angle%modes[mode][4]==0):
            time.sleep(1)
        else:
            time.sleep(0.1)

draw()            
master.after(0,animate) #start animating in 0 seconds on background thread
mainloop() #main loop
