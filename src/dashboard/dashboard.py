import zmq
import libconf
import msgB_pb2 as msgB

import os

def _DashBoardPrint(msg: msgB.DashboardMessage):
    #clear console
    os.system('clear')

    #now print the message
    print("----- Dashboard Message -----")

    #recycled trash per planet
    print("PLANETS (Recycled Trash)")
    for i, amount in enumerate(msg.recycled_trash):
        print(f" {chr(65 + i)} - {amount}")
    
    #trash in each ship cargo
    print("TRASH SHIPS (Trash Cargo)")
    for i, amount in enumerate(msg.ship_cargo):
        print(f" {chr(65 + i)} - {amount}")

    #universe
    print("UNIVERSE")
    print(f"ROAMING TRASH {msg.roaming_trash}")

    trash_percentage = int(msg.roaming_trash/msg.max_trash_capacity * 100)

    print(f"TRASH CAPACITY {trash_percentage}%")

if __name__ == "__main__":

    config_path = "dashboard-config.conf"

    config = libconf.load(open(config_path))
    port = config.dashboard_ports.port

    context = zmq.Context.instance()
    socket = context.socket(zmq.SUB)
    socket.connect(f"tcp://localhost:{port}")
    socket.setsockopt_string(zmq.SUBSCRIBE, "") #subscribe to all messages
    socket.setsockopt(zmq.RCVTIMEO, 1000) #timeout in ms
    print(f"Dashboard server is running on port {port}...")

    while True:
        try:
            message = socket.recv()
        except zmq.Again:
            continue
        
        #process message with protobuf
        msg = msgB.DashboardMessage()
        msg.ParseFromString(message)

        _DashBoardPrint(msg)
