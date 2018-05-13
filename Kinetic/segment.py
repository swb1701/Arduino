from Tkinter import *
import math

class Segment:
    
    def __init__(self, canvas, x, y, w, h, angle):
        self.canvas=canvas
        self.x=x
        self.y=y
        self.w=w
        self.h=h
        self.angle=angle
        self.pts=[[x-w/2,y-h/2],[x+w/2,y-h/2],[x+w/2,y+h/2],[x-w/2,y+h/2]]

    def rotate(self, points, angle, center):
        angle = math.radians(angle)
        cos_val = math.cos(angle)
        sin_val = math.sin(angle)
        cx, cy = center
        new_points = []
        for x_old, y_old in points:
            x_old -= cx
            y_old -= cy
            x_new = x_old * cos_val - y_old * sin_val
            y_new = x_old * sin_val + y_old * cos_val
            new_points.append([x_new + cx, y_new + cy])
        return new_points

    def update(self,angle):
        self.angle+=angle
        
    def draw(self):
        self.canvas.create_polygon(self.rotate(self.pts,self.angle,(self.x,self.y)),outline="black",fill="",width=1)

