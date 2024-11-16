# udp_server
udp_server for school task 

### Hours spent
14h 15min


### TODO
- Add -x command line argument to ReadME
- Continue with Reliable data transfer with only positive ACKs
- Reliable data transfer with only negative ACKs

### Command-Line Arguments
The application accepts the following command-line arguments:

| Argument                            | Description                            | Shorthand |
|-------------------------------------|----------------------------------------|-----------|
| Port number                         | The port your application uses (default: `6666`)        | `-p`      |
| Probability for packet delay        | Delay probability (0.0 to 1.0)         | `-d`      |
| Probability for packet drop         | Drop probability (0.0 to 1.0)          | `-r`      |
| Delay in milliseconds               | Delay time in ms                       | `-t`      |

### Default Values
- **Port**: If the `-p` argument is not provided, the default port number will be `6666`.
- **Other**: If arguments for probability and delay is not provided, the default values will be `0`.

### Example Usage

```bash
./udp_server -p 1243 -d 0.1 -r 0.05 -t 100

- -p 1243: Sets the port number to 1243. (default 6666)
- -d 0.1: Sets the probability for packet delay to 0.1.
- -r 0.05: Sets the probability for packet drop to 0.05.
- -t 100: Sets the delay in milliseconds to 100 ms.
