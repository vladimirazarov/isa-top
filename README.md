# conntop

A console application to display current network transfer speeds for communicating IP addresses in real-time.

## Description

`conntop` monitors network traffic on a specified interface using `libpcap` and displays connection statistics in the terminal using `ncurses`. It uses a multi-threaded approach for packet capture and display updates.

## Building

```bash
make
```

## Running

Run with root privileges:

```bash
sudo ./conntop -i <interface> [-s <sort_by>] [-l]
```

*   `-i <interface>`: Network interface to capture packets from (required).
*   `-s <sort_by>`: Sort criteria (bytes or packets). Defaults to bytes.
*   `-l`: Enable logging to `log.csv` in the current directory.

## Testing

Requires Google Test framework.

```bash
# Build tests
make unit_tests
make integration_tests

# Run tests
./unit_tests
./integration_tests
```

## Project Structure

*   `src/`: Source code files.
*   `test/`: Unit and integration tests.
*   `Makefile`: Build script.
*   `manual.pdf`: Detailed documentation (in Czech).
*   `conntop.1`: Man page.
*   `LICENSE`: Project license. 