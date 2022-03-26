
# Initial Install (based on Raspbian bullseye)
```bash
cd /srv
mkdir Bramnik && chown pi:pi Bramnik
git clone https://github.com/minsk-hackerspace/Bramnik.git
sudo apt-get install python3-pip python3-smbus python3-click python3-peewee moreutils
pip3 install smbus2
cd Bramnik/software/host
```

# run

```bash
# edit service/bramnik.env, fill needed variables
. ./service/bramnik.env
./host.py  --loglevel $BRAMNIK_LOGLEVEL --tgtoken $TELEGRAM_TOKEN --tgchatid $TELEGRAM_NOTIFICATIONS_CHAT
```

# install as service (systemd)

```bash
sudo cp service/bramnik.service /etc/systemd/system/
# fix paths in the bramnik.service file if necessary
sudo mkdir /etc/bramnik
sudo cp service/bramnik.env /etc/bramnik/
sudo systemctl start bramnik # start bramnik right now
sudo systemctl enable bramnik # start bramnik at system start
```

# Cron to resync users and cards and emit code for open day

```
sudo cp etc_cron.d/bramnik /etc/cron.d
sudo chown root:root /etc/cron.d/bramnik
sudo systemctl restart cron
```
