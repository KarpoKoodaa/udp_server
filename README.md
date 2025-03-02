# udp_server
udp_server for school task 
http://users.jyu.fi/~arjuvi/opetus/ties322/2018/demot.html

### Hours spent
72h 00min


### TODO
- Finalize documentation 

### Command-Line Arguments
The application accepts the following command-line arguments:

| Argument                            | Description                            | Shorthand |
|-------------------------------------|----------------------------------------|-----------|
| RDT Version                         | RDT version to use 1.0, 2.0, 2.1, 2.2 or 3.0 | `-x`|
| Go-Back-N                           | Go-Back-N Server Mode                  | `-g`      |
| Selective Repeat                    | Selective Repeat Server Mode           | `-g`      |
| Port number                         | The port your application uses (default: `6666`)        | `-p`      |
| Probability for packet delay        | Delay probability (0.0 to 1.0)         | `-d`      |
| Probability for packet drop         | Drop probability (0.0 to 1.0)          | `-r`      |
| Delay in milliseconds               | Delay time in ms                       | `-t`      |

### Default Values
- **Port**: If the `-p` argument is not provided, the default port number will be `6666`.
- **RDT**: If the `-x`argument is not provided, the default rdt will be `rdt 1.0`
- **GBN**: If the `-g`argument is not provided, the default will be RDT mode.
- **SR**: If the `-g`argument is not provided, the default will be RDT mode.
- **Other**: If arguments for probability and delay is not provided, the default values will be `0`.

### Example RDT Usage 

```bash
./udp_server -x 2.2 -p 1243 -d 0.1 -r 0.05 -t 100

- -x 2.2: Sets RDT version 2.2
- -p 1243: Sets the port number to 1243. (default 6666)
- -d 0.1: Sets the probability for packet delay to 0.1.
- -r 0.05: Sets the probability for packet drop to 0.05.
- -t 100: Sets the delay in milliseconds to 100 ms.
```

### Example GBN Usage
```bash
./udp-server -g -r 0.5

- -g : Starts the server with Go-Back-N mode
- -r 0.5: Sets the probability for packet drop to 0.5
```

## Run the Server
```bash
build/udp-server -s -r 0.1

- -s : Starts the server with Selective Repeat mode
- -r 0.1: Sets the probability for packet drop to 0.1
```

# Go-Back-N
The gbn_client.c is a Go-Back-N (GBN) UDP client implemented in C. It provides a reliable data transfer mechanism over an unreliable UDP connection by handling packet loss, retransmissions, and acknowledgments. This implementation ensures that packets are delivered in order and without corruption by utilizing CRC-based error checking.

** Supports only port 6666 (Default port of the server)**

## Run the Client
``` bash
./sr_client

```

### How Go-Back-N Works in This Client
1. Divides data into packets, each assigned a unique sequence number
2. Sends multiple packets in a sliding window (default: 5 packets at a time)
3. Waits for ACK/NACK responses from the server
4. If an ACK is received, the the window base is the received ACK's sequence number
5. If timeout occurs, all packet starting from base is resent. 
6. The process repeats until all packets are successfully acknowledged


### Example Log Output
```sql
Configuring remote address...
Remote address is: 127.0.0.1 6666
Creating socket...
Connecting...
Connected.

Ready to send data to server
----- Sending Packet 1 -------
Packet sent: SEQ 1 | Data: H | Bytes: 3
----- Packet Send End -------

----- Packet Receive Start -------
ACK received: SEQ 1 | Data: A | CRC Check: OK
----- Packet Receive End -------

----- Timeout occurred -------
Window base: 5 | Next SEQ: 5
----- Timeout end -------
```

# Selective Repeat
 Selective Repeat UDP Client implemented in C, designed to reliably transmit messages over UDP while handling packet loss and reordering.

**Selective Repeat client supports only port 6666 (default port of server)**

## Run the Client
``` bash
./sr_client

```
### How Selective Repeat Works in This Client
1. Divides data into packets, each assigned a unique sequence number
2. Sends multiple packets in a sliding window (default: 5 packets at a time)
3. Waits for ACK/NACK responses from the server
4. If an ACK is received, the packet is marked as delivered
5. If an NACK or timeout occurs, only the missing packets are retransmitted
6. The process repeats until all packets are successfully acknowledged

### Example Log Output
```sql
Configuring remote address...
Remote address is: 127.0.0.1 6666
Creating socket...
Connected.

Ready to send data to server
----- Sending Packet 1 -------
Sent 3 bytes. Data: H
----- Packet Send End -------

----- Packet Receive Start -------
ACK Received for packet 1
Sliding Window: Base=2, Next Expected=3, Window Size=5
----- Packet Receive End -------

----- Timeout occurred -------
----- Resending Packet 3 -------
Sent 3 bytes. Data: l
----- Packet Resend End -------
```
