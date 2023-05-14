# Duino-AVR-rig
A powerful tool having the ability to emulate an entire Duino-Coin AVR mining rig
# DISCLAIMER
This tool has been made for testing purposes only as it is very powerful. Using this tool to mine will most likely get you banned. Use this tool at your own risk. I am **not** responsible for any damage to your Duino-Coin account.
## Building
### Requirements
* g++
* make
* openssl (libssl-dev)
* curl (libcurl4-openssl-dev)

To build Duino-AVR-rig navigate to the root of this project and run `make`. All output files will be stored under the `build/` directory.


## Using Duino-AVR-rig
`build/duino-avr-rig Username MiningKey Hashrate RigIdentifier MinerThreads`

* Replace the first argument with your username
* Replace the second argument with your mining key (or "None")
* Replace the third argument with the desired hashrate of each worker in H/s
* Replace the fourth argument with a rig identifier (or "None")
* Replace the fifth argument with the number of AVRs

## License
All files within this repo are released under the GNU GPL V3 License as per the LICENSE file stored in the root of this repo.
