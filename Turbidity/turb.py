import serial
import datetime

ser=serial.Serial('/dev/ttyACM0',9600)

with open('turbidity.log','a') as f:
    while True:
            d=datetime.datetime.now().ctime()
            v=ser.read_until('\n')
            line=d+','+v.strip()+'\n'
            f.write(line)
            f.flush()
            print(line)

