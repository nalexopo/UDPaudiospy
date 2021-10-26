import logging
import socket
import pyaudio

p = pyaudio.PyAudio()

stream = p.open(format=pyaudio.paInt32,#SPH0645 supposedly gives 18 useful bits per sample but we send them inside 32 bit frames from I2s. It works nevertheless.
                channels=1,
                rate=16000,#16kHz sample rate
                output=True)


log = logging.getLogger('udp_server')


def udp_server(host='XXX.XXX.XXX.XXX', port=XXXX):
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    
    log.info("Listening on udp %s:%s" % (host, port))
    s.bind((host, port))
    
    
    (data, addr) = s.recvfrom(1024)
    while True:
        stream.write(data)
        (data, addr) = s.recvfrom(1024)
        yield data


FORMAT_CONS = '%(asctime)s %(name)-12s %(levelname)8s\t%(message)s'
logging.basicConfig(level=logging.DEBUG, format=FORMAT_CONS)

for data in udp_server():
    log.debug("%r" % (data,))