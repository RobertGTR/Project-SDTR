import socket
import pyaudio

# Audio format settings
FORMAT = pyaudio.get_format_from_width(2, False)  # 16-bit PCM format
CHANNELS = 1  # Mono audio
RATE = 24000  # Sample rate 24kHz
CHUNK = 265  # Number of samples per chunk

# Socket settings
HOST = '192.168.179.37'  # Replace with your MCU's IP address
PORT = 8080  # Replace with your port number

# Initialize PyAudio
audio = pyaudio.PyAudio()

# Open a stream for audio playback
stream = audio.open(format=FORMAT, channels=CHANNELS, rate=RATE, output=True)

print("Opened audio stream\n")

# Create a socket object
client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

print("Connecting to " + HOST + ":" + str(PORT) + "...")

# Connect to the server (MCU)
client_socket.connect((HOST, PORT))

print("Connected to server!\n")

try:
    while True:
        # Receive data from the socket
        data = client_socket.recv(2 * 20)

        if not data:
            break

        # Play the received audio data
        stream.write(data)


except KeyboardInterrupt:
    pass
finally:
    # Stop and close the stream
    stream.stop_stream()
    stream.close()

    # Terminate the PortAudio interface
    audio.terminate()

    # Close the socket
    client_socket.close()

print("Streaming stopped")
