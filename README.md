# PF-Project
A Chat Application using C language 

* HOW TO RUN
* To run the chat application type the below command in your terminal
* gcc server.c -o server.exe -lws2_32
- Press Enter, this will compile the server,then write below command
- gcc client.c -o client.exe -lws2_32
* Press Enter, this will compile the clients, then type:
* .\server.exe
* The server will start, then open a new terminal in the same folder or copy the client code any where in the pc/laptop, compile it and type the following command and you will * be able to connect to server and chat with others on the server:
- .\client.exe
- open 2-3 different terminal and add clients and test the chat application.
- if you wish to communicate with people on diiferent pcs and laptops first ensure you all are connected to the same Wifi, then 
* In client.c, intead of localhost ip address: 127.0.0.1 , enter the device ip address on which the server is running, it would look like 192.168.X.X,
* by this you will even communicate with people on different devices 
