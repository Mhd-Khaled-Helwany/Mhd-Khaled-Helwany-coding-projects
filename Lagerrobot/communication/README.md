# Kommunikationsmodulen

Skriven i python, körs på raspberry PI:n

## Updating files on raspberry pi

Connect to the raspberry pi:s wifi network called "bezos". The raspberry pi itself has a static ip address of 192.168.4.1.

For the WiFi-password, ask Albin. For the password to the g08 account on the raspberry pi, ask Albin. Or ask to have your ssh key added!

Run `rsync -r COMMUNICATION_DIR g08@192.168.4.1:/var/communication` to move the files over to the correct directory.

The program *should* run automatically as well as restart when new files has been synced to the rpi, otherwise run `sudo systemctl restart communication`.

To watch the output of the service run `sudo journalctl -fu communication`.

