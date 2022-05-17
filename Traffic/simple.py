import boto3
import json

key='<your_access_key>'
sec='<your password>'
session=boto3.Session(
    aws_access_key_id=key,
    aws_secret_access_key=sec
)

iot=session.client('iot-data',region_name='us-east-1')
iot.publish(
    topic='inTopic',
    qos=1,
    retain=False,
    payload='traffic:yellow'
)
