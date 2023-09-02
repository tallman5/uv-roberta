#!/bin/sh


cd ~


sudo apt update && sudo apt upgrade -y


echo -e "\e[32mInstalling git...\e[0m"
sudo apt install -y git


echo -e "\e[32mFix for missing /dev/serial/by-id...\e[0m"
sudo cp /usr/lib/udev/rules.d/60-serial.rules /usr/lib/udev/rules.d/60-serial.old
sudo wget -O /usr/lib/udev/rules.d/60-serial.rules https://raw.githubusercontent.com/systemd/systemd/main/rules.d/60-serial.rules


echo -e "\e[32mInstalling FTP...\e[0m"
sudo apt install -y vsftpd
sudo cp /etc/vsftpd.conf /etc/vsftpd.old
sudo sed -i 's/#write_enable=YES/write_enable=YES/' '/etc/vsftpd.conf'
sudo service vsftpd restart


echo -e "\e[32mInstalling pigpio...\e[0m"
sudo apt install -y pigpio


echo -e "\e[32mInstalling ustreamer...\e[0m"
sudo apt install -y libjpeg-dev libevent-dev libbsd-dev nlohmann-json3-dev libwebsockets-dev
git clone --depth=1 https://github.com/pikvm/ustreamer
cd ustreamer/
make
cd ~


echo -e "\e[32mConfiguring ustreamer service...\e[0m"
sudo useradd -r ustreamer
sudo usermod -a -G video ustreamer
sudo cp ~/bootstrap/roberta/ustreamer@.service /etc/systemd/system/ustreamer@.service
sudo systemctl enable ustreamer@.service
sudo systemctl enable ustreamer@0.service


echo -e "\e[32mInstalling .NET...\e[0m"
curl -sSL https://dot.net/v1/dotnet-install.sh | bash /dev/stdin --channel 6.0.4xx


echo -e "\e[32mUpdating .bashrc...\e[0m"
echo '' >> ~/.bashrc
echo 'export DOTNET_ROOT=/home/pi/.dotnet' >> ~/.bashrc
echo 'export PATH=$PATH:$HOME/.dotnet' >> ~/.bashrc
source ~/.bashrc


echo -e "\e[32mInstalling snapd...\e[0m"
sudo apt install snapd -y
sudo snap install core


echo -e "\e[32mInstalling certbot...\e[0m"
sudo snap install --classic certbot
sudo ln -s /snap/bin/certbot /usr/bin/certbot
sudo certbot certonly --standalone
sudo certbot renew --dry-run


echo -e "\e[32mCopying certs...\e[0m"
mkdir certs
mkdir certs/rofo
sudo cp /etc/letsencrypt/live/rofo.mcgurkin.net/fullchain.pem ~/certs/rofo/
sudo cp /etc/letsencrypt/live/rofo.mcgurkin.net/privkey.pem ~/certs/rofo/
cd ~/certs/rofo
sudo chmod +r fullchain.pem
sudo chmod +r privkey.pem

# This is needed for easy access for the .NET API/Hub
echo -e "\e[32mInstalling nginx...\e[0m"
sudo apt-get install nginx -y
sudo cp ~/bootstrap/roberta/ustreamer-proxy /etc/nginx/sites-available/ustreamer-proxy
sudo ln -s /etc/nginx/sites-available/ustreamer-proxy /etc/nginx/sites-enabled/
sudo nginx -t
sudo systemctl reload nginx


echo -e "\e[32mConfiguring Roberta Hub service...\e[0m"
sudo cp ~/bootstrap/roberta/robhub.service /etc/systemd/system/robhub.service
sudo systemctl enable robhub.service


echo -e "\e[32mRebooting...\e[0m"
sudo reboot now
