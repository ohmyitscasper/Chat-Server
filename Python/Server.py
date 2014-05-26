import socket
import threading
import signal
import sys

#Defining some constants
BLOCK_TIME = 60


users = []
loginInfo = {}
blockedAddresses = {}
serverSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

def signal_handler(signal, frame):
        print "Ctrl+C pressed....Exitting cleanly..."
        serverSocket.close()
        sys.exit(0)


class userThread (threading.Thread):
	def __init__(self, threadID, socket, username, address, loggedIn):
		threading.Thread.__init__(self)
		self.threadID = threadID
		self.socket = socket
		self.username = username
		self.address = address
		self.loggedIn = loggedIn

	def messageHandler(self, command):

	def run(self):
		sendStr = "Welcome " + self.username
		self.socket.send(sendStr)
		while True:
			self.socket.send('\nPlease say a command: ')
			string = self.socket.recv(1024)

			#Sanitize the string and send it to the message handler
			string = string.rstrip()




class socketThread (threading.Thread):
	def __init__ (self, threadID, socket, address):
		threading.Thread.__init__(self)
		self.threadID = threadID
		self.socket = socket
		self.address = address
	
	def loginHandler(self, username, password):
		username = username.rstrip()
		print username
		password = password.rstrip()
		if username in loginInfo.keys():
			print 'Key found'
			if password == loginInfo[username]:
				return True
			else:
				return False
		else:
			print 'Key not found'
			return False
	
	def run(self):
		print "Thread started with socket to ", self.address
		i = 1
		userThreadNum = 1
		while i < 4:
			#Sending prompts for the username/password
			self.socket.send("Username: ")
			string = self.socket.recv(1024)
			username = string
			self.socket.send("Password: ")
			string = self.socket.recv(1024)
			password = string

			#Call the login handler
			loggedIn = self.loginHandler(username, password)

			if loggedIn:
				loginFailed = False
				break
			else:
				sendString = 'Incorrect Attempt # %d\n' % (i)
				self.socket.send(sendString)

			i = i+1
			if i==4:
				loginFailed = True

		if loginFailed:
			self.socket.send("3 login attempts. Booted...\n")
			self.socket.close()
		else:
			self.socket.send("Successful login...\n")
			thread = userThread(userThreadNum, self.socket, username, self.address, True)
			userThreadNum += 1
			thread.daemon = True
			thread.start()
			users.append(thread)



def main():

	total = len(sys.argv)
	if total < 2:
		print "Wrong commandline args"
		sys.exit(0)

	port = int(sys.argv[1])

	#Handle reading in file now
	filename = 'users.txt'
	f = open(filename)
	for line in f:
		lineSplit = line.split()
		username = lineSplit[0]
		password = lineSplit[1]
		loginInfo[username] = password


	serverSocket.bind(('localhost', port))
	serverSocket.listen(5)

	signal.signal(signal.SIGINT, signal_handler)

	threadNum = 1

	print "Waiting for connections"

	while True:
		conn,adr = serverSocket.accept()
		thread = socketThread(threadNum, conn, adr)
		threadNum += 1
		thread.daemon = True
		thread.start()
	print "Done"

if __name__ == "__main__":
	main()
