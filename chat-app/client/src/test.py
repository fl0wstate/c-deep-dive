# 10 clients
# We will use threads
# Keep a connection open for 10 data exchange (sleep 1 second between them)
# Keep closing and opening new connections to the server
# Log out the answer from the server

from logging.handlers import SysLogHandler
import logging
from threading import Thread, Lock
import signal
import time
import socket

def get_logger():
    handler = SysLogHandler()
    formatter = logging.Formatter('client: %(message)s')
    logger = logging.getLogger(__name__)
    handler.setFormatter(formatter)
    logger.addHandler(handler)
    logger.setLevel(logging.INFO)
    return logger


logger = get_logger()

stopping = False
lock = Lock()


def signal_handler(sig, frame):
    with lock:
        global stopping
        stopping = True


def client_start(i):
    time.sleep(i)
    while True:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        server_address = ('localhost', 7000)
        logger.info(f"Client {i} opens a new connection")
        sock.connect(server_address)
        for j in range(10):
            time.sleep(1)
            with lock:
                if stopping:
                    sock.close()
                    return
            try:
                sock.sendall(b"PING\n")
                data = sock.recv(512)
                logger.info(f"Client {i} got answer: {data.decode()}")
            except Exception as e:
                logger.exception(e)
                break
        sock.close()


def main():
    signal.signal(signal.SIGTERM, signal_handler)
    signal.signal(signal.SIGINT, signal_handler)
    logger.info("Starting the clients")
    threads = [Thread(target=client_start, args=(i,)) for i in range(10)]
    [t.start() for t in threads]
    while True:
        with lock:
            if stopping:
                break
        time.sleep(0.5)
    [t.join() for t in threads]
    logger.info("Exiting")


if __name__ == '__main__':
    main()
