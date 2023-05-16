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
`build/duino-avr-rig wallet.txt`

* Replace the first argument with the config file

## Writing a config file
The multi account configuration file is pretty straightforward. 

* Each line configures the parameters for one account
* Every parameter is split by '?'

To make your own, copy or just edit the example config `wallets.txt` and adjust it to your liking.

## License
All files within this repo are released under the GNU GPL V3 License as per the LICENSE file stored in the root of this repo.
