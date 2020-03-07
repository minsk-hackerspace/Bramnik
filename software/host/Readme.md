
# Initial Install
1. sudo apt-get install python-pip
1. sudo apt-get install python3-smbus
1. pip install virtualenv
1. sudo /usr/bin/easy_install virtualenv
1. cd <project_path>

# install what's needed
1. cd <project_path>
1. virtualenv -p python3 env
1. source env/bin/activate
1. pip install -r requirements.txt

# run

```bash
env/bin/python host.py
```

#install as service (systemd)

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
