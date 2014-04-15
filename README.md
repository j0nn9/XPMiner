# XPMiner - A standalone Primecoin (XPM) CPU pool miner
---
<br/>
## So why an other Primecoin miner? 


  * XPMiner is fully open source (GPLv3) while the most other miner
    are closed source (at least the newer versions)

  * XPMiner's intension is to provide a readable and clean implementation
of the Primecoin mining algorithm beside the whole Primecoin client.

  * pure C code and only pthread, openssl and gmp dependencies

  * highly configurable

  * implementation of xolominers getwork-protocol (with longpoll-support)


## get it running
---

First of all keep in mind that XPMiner still has alpha qualities and 
doesn't claims to be the fastest CPU miner out there, the main focus
is read- and understandability of the Primecoin mining algorithm!

Also it's currently Linux only, sorry.

### required libraries
  - pthread
  - openssl
  - gmp 

### installation
```sh
  git clone https://github.com/j0nn9/XPMiner.git
  cd XPMiner
  make all
  make install
```
## Usage
---

  `xpminer [--options]`

### basic

 - `--pool-ip  [ipv4]` ipv4 address of the pool

 - `--pool-port  [port]` pool port to connect to

 - `--pool-user  [user]` pool user name (this is normally your XPM address)

#### example (beeeeer.org):

`xpminer --poolip 176.34.128.129 -pool-port 1337 --pool-user AWtYkHdxoXwUuQGsCNCRHXfYxoq2ZaKTWq`

### advanced

  - `--pool-free  [NUM]` the fee in percent (1-100) 

  - `--pool-pwd  [STR]` pool password (will be send sha1 encrypted)

  - `--num-threads  [NUM]` number of threads to use

  - `--miner-id  [NUM]` give your miner an id (0-65535) 

  - `--sieve-extensions [NUM]` the number of sieve extension to use 

  - `--sieve-primes  [NUM]` the number of primes to sieve              

  - `--sieve-size  [NUM]` the sieve size 

  - `--primes-in-hash  [NUM]` the number of primes the hash should be divisible by

  - `--primes-in-primorial [NUM]` the number of additional primes multiplied to the header hash

  - `--chain-length  [NUM]` the chain length you are mining for        

  - `--cache-bits  [NUM]` the number bits to sieve at once (cache optimization)

  - `--stats-interval  [NUM]` interval in seconds to print mining statistics

  - `--pool-share  [NUM]` smallest share credited by your pool        

<br/>
## How to contribute:
---

### donate:

  - `[XPM] AWtYkHdxoXwUuQGsCNCRHXfYxoq2ZaKTWq`
  - `[LTC] LKaqVjAuKAuhLfH5mSixs1JG3bsu3HnKQy`
  - `[BTC] 19EiHLDfNUhukypDN5GVFygZw34hTNaBj5`

### Or better:

  - test the miner
  - read the source code
  - find a bug and make a pull request
