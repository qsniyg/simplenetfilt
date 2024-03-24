# simplenetfilt

This is a simple project to disable outside network access for linux programs (primarily developed for usage with wine). It optionally allows loopback (`localhost` / `127.0.0.1`) and local network access (192.168.\*.\*, 10.\*.\*.\*, \*.local).

This was created out of a need to run some programs under wine without outside network access. Loopback was needed for some IPC work the programs required. After spending days wrestling with `firejail`, X11, and `iptables`, I gave up and decided to write this.

## Current limitations

 * Only outgoing connections are filtered.
 * IPv6 isn't supported. Currently it blocks all outgoing IPv6 connections.
 * Only tested under wine and a few basic programs (curl, ping, firefox). Leaks may exist for other software.
 * No whitelist/blacklist.

Please open an issue if these limitations cause a problem for you.

## Usage

### Building

```sh
mkdir build && cd build
cmake ..
make
sudo make install
```

### Running

Using the wrapper:

```sh
simplenetfilt program [args...]
```

Manual LD_PRELOAD:

```sh
LD_PRELOAD=libsimplenetfilt.so program [args...]
```

Note that when using the manual method, if it has been installed to `/usr/local/lib`, you may need to add `/usr/local/lib` to `LD_LIBRARY_PATH`.

### Configuration

The following environment variables can be configured:

 * `SIMPLENETFILT_ALLOW_LOCALHOST` - Allows `localhost` / `127.0.0.1` access. Default: `true`
 * `SIMPLENETFILT_ALLOW_LOCALNET` - Allows local network access. Default: `false`
