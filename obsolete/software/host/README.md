### Copy to remote host 
`scp -P 222 * pi@bramnik:/home/pi/Bramnik/software/host`

### To run
##### Directly
`~/bramnik/software/host: ruby host.rb`

##### With self-crash monitor as daemon
`god -c ~/bramnik/software/host/bramnik.god`

##### With self-crash monitor foreground
`god -c ~/bramnik/software/host/bramnik.god -D`

##### Restart with monitor
`god restart bramnik`