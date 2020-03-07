
# Initial Install
```bash
sudo apt-get install python-pip
sudo apt-get install python3-smbus
pip install virtualenv
sudo /usr/bin/easy_install virtualenv
cd <project_path>
```

# install what's needed
```bash
cd <project_path>
virtualenv -p python3 env
source env/bin/activate
pip install -r software/host/requirements.txt
```

# run

```bash
env/bin/python host.py
```

# install as service (systemd)

```bash
sudo systemctl edit --force bramnik
# fill it with contents of service/bramnik.service
# fix paths if necessary
sudo mkdir /etc/bramnik
sudo nano /etc/bramnik/bramnik.env
#fill it with service/bramnik.env
sudo systemctl start bramnik # start bramnikright now
sudo systemctl enable bramnik # start bramnik at system start
```


# Todo
* Make date comparison in UTC
