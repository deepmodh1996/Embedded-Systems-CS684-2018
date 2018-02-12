import serial
from matplotlib import pyplot as plt

SERIAL_PORT = '/dev/ttyACM0'
SERIAL_RATE = 57600

ser = serial.Serial(SERIAL_PORT, SERIAL_RATE)

plt.axis([-1, 256, -1, 256])
plt.ion()
while True:
	try:
		reading = ser.readline()
		if len(reading) != 4:
			print(len(reading), reading)
		else: # expecting input of the format 'X' ',' 'Y' and '\n'
			print('(',reading[0],',', reading[2],')', reading)
			plt.cla()
			plt.axis([-1, 256, -1, 256])
			plt.scatter(255-reading[0], reading[2])
			plt.pause(0.00005)
	except KeyboardInterrupt:
		print('KeyboardInterrupt ... Exiting')
		break
# close serial
ser.flush()
ser.close()
