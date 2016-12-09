"# CSC361-Simple-Web-Server" 
"# CSC361" 
CSC361 P1

Zihan Ye
V00793984
2015.10.12

The sturucture of my code includes four major parts:


1. main function: including all useful functions.

2. initialization: trying to socket and bind the IP address.

3. while loop: repeatly calling the recvfrom function.

4. receiveFrom: checking if the input is a legal language or not;
			if legal: sengding back information 200 or 404
			if illegal: sending back information 400 
