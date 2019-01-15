import datetime

with open('turbidity.log','a') as f:
    while True:
        label=raw_input("Jar-->")
        d=datetime.datetime.now().ctime()
        line=label+','+d+'\n'
        f.write(line)
        f.flush()
        print(line)

