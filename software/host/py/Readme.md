
# Initial install (system packages)

```bash
sudo apt-get install python-pip
sudo pip install virtualenv
sudo /usr/bin/easy_install virtualenv
```

# Install required python packages (to virtual env)

``` bash
cd <project_path>
virtualenv -p python3 env
source env/bin/activate
pip install -r requirements.txt
```

# Run (for development and/or debug)

```bash
env/bin/python host.py # --loglevel DEBUG # as optional arg

# or using virtualenv activation:

source env/bin/activate # once at session start
python host.py # --loglevel DEBUG # as optional arg

```



# How to install as service (systemd)

```bash
sudo systemctl edit --force bramnik
# fill it with contents of service/bramnik.service
# fix paths if necessary
sudo mkdir /etc/bramnik
sudo nano /etc/bramnik/bramnik.env
#fill it with service/bramnik.env
sudo systemctl start bramnik # start bramnik right now
sudo systemctl enable bramnik # start bramnik at system start

```
