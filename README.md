# udp_server
udp_server for school task 

### Hours spent
52h 00min


### TODO
- UDP client instructions
- Make instructions
- Next phase

### Command-Line Arguments
The application accepts the following command-line arguments:

| Argument                            | Description                            | Shorthand |
|-------------------------------------|----------------------------------------|-----------|
| RDT Version                         | RDT version to use 1.0, 2.0, 2.1, 2.2 or 3.0 | `-x`|
| Go-Back-N                           | Go-Back-N Server Mode                  | `-g`      |
| Port number                         | The port your application uses (default: `6666`)        | `-p`      |
| Probability for packet delay        | Delay probability (0.0 to 1.0)         | `-d`      |
| Probability for packet drop         | Drop probability (0.0 to 1.0)          | `-r`      |
| Delay in milliseconds               | Delay time in ms                       | `-t`      |

### Default Values
- **Port**: If the `-p` argument is not provided, the default port number will be `6666`.
- **RDT**: If the `-x`argument is not provided, the default rdt will be `rdt 1.0`
- **GBN**: If the `-g`argument is not provided, the default will be rdt mode.
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
./udp-server -g -p 1234 -r 0.5

- -g : Starts the server with Go-Back-N mode
- -p 1234: Sets the port to 1234 (default 6666)
- -r 0.5: Sets the probability for packet drop to 0.5
```

