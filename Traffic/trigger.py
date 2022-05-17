from python_command import PythonCommand #part of a slack bot command framework I wrote
import boto3
import traceback

class Code(PythonCommand): #command is "code <color>"

    def run_command(self,context,command,util):
        color=command.split(" ")[1].strip()
        try:
            keys=util.config.annunciator_keys.split(":") #gets access key, secret, and endpoint
            session=boto3.Session(
                aws_access_key_id=keys[0],
                aws_secret_access_key=keys[1],
                region_name="us-east-1"
            )
            iot=session.client('iot-data',endpoint_url="https://"+keys[2])
            #Note: client needs AmazonRootCA1 or it won't work
            iot.publish(
                topic='inTopic', #topic
                qos=1, #quality of service (1 means deliver "at least" once)
                retain=False, #don't retain the message
                payload=f"traffic:{color}" #payload giving the color
            )
            util.slack.post_text(context,f"We've gone to code '{color}'")
        except:
            print(traceback.format_exc())
            util.slack.post_text(context,f"Sorry, unable to set status to '{color}' -- see logs")
