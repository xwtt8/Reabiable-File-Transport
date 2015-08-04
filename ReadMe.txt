Group members:
Xiaotian Wang  xw31
Zhongjie Chen  zc22
Xiewen Wen     xw8

Packet Formats:
Our ping pong packet format is like:
uint8_t ----------------- checksum (md5)|packet_type|time_sec|time_usec|file_length 

Our sendfile packet format is like:
uint8_t ----------------- checksum (md5)|packet_type|time_sec|time_usec|seq_num|data_payload 

Our Ack packet format is like:
uint8_t ----------------- checksum (md5)|ack_num|time_sec|time_usec
                          

Protocols and algorithm we use:
Our send and receive files use the UDP's sendto() and receivefrom() function. Basically, we use UDP unreliable datagram protocol.
For the algorithm part, we use the md5 to check the correctness of our packet. 

Features of our design:
To deal with the delay, drop, reorder, mangle and duplicate error during the transmission, our design has following features.

(1) To deal with the reorder problem, we use a window in the receiver to store the data packets according to their sequence number and write the every file segment to the new file in the sequence according to their sequence numbers. 

(2) To deal with the drop problem, we use the time out to figure out whether we should retransmit the packet to the receiver.

(3) To deal with the duplicate problem, we have a array of sequence number and check whether we have received this sequence number before. The receiver will send acknowledgement to the send after receiving a duplicate sequence number and the sender can update its sliding window accordingly.
 
(4) To deal with the mangle problem, we use the md5 to check the packet we receive, and if we find that the packet was mangled, we just drop it. 

(5) To deal with the delay problem, we just wait it until time out.  

Examples of how to run our code: 
First, using the command "make" to compile our code.
Then, we first run the recvfile. For example, if you want to use the port number 18001, then the command should like “./recvfile -p 18001".
And we should run the sendfile(we made assumption that you have file to send). Then, sendfile should run on cai.cs.rice.edu.
Then the command should be like "./sendfile -r ip_address:port_number -f file_name " .

Other things to clarify:
Notice on the size of the file being transfer:
	Since we are doing one time transfer, we just use one sequence number space with an array of size 50000 and not wrap around(our sequence number is base on the count of packet not byte). we set the data payload in every packet to be 2048bytes, so right now it can transfer 100MB file. We have tested our FTP with a 4MB file and it can be transferred within 10s in average when all the setting are set to 50. Theoretically, if we want to transfer more data, we just need to extend the sequence number space. 

Testing method:
	We use a hash function in sha-1.c to test the correctness of the received packet by comparing the generate string from the original file and the received file. It turns out the received file is identical to the original file when transferred under the condition that all the parameters are set to 50.