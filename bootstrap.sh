#!/bin/sh

# Run for prerequisites
# sudo apt update
# sudo apt upgrade -y
# sudo apt install git -y
# git clone https://github.com/tallman5/uv-roberta.git

# Upload two secrets.json files, one for Hub and one for Client 

# bash ~/uv-roberta/bootstrap.sh

cd ~


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
sudo cp ~/uv-roberta/src/ustreamer/ustreamer@.service /etc/systemd/system/ustreamer@.service
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


echo -e "\e[32mInstalling nginx...\e[0m"
sudo apt-get install nginx -y
sudo cp ~/uv-roberta/src/ustreamer/ustreamer-proxy /etc/nginx/sites-available/ustreamer-proxy
sudo ln -s /etc/nginx/sites-available/ustreamer-proxy /etc/nginx/sites-enabled/
sudo nginx -t
sudo systemctl reload nginx


echo -e "\e[32mBuilding Hub...\e[0m"
cd ~/uv-roberta/src/Roberta.Hub/Roberta.Hub
~/.dotnet/dotnet publish -c Release -r linux-arm64 -o ~/roberta/hub --self-contained false -p:PublishSingleFile=true

echo -e "\e[32mConfiguring Roberta Hub service...\e[0m"
sudo cp ~/uv-roberta/src/Roberta.Hub/Roberta.Hub/robhub.service /etc/systemd/system/robhub.service
sudo systemctl enable robhub.service


echo -e "\e[32mBuilding Client...\e[0m"
cd ~/uv-roberta/src/Roberta.Client/Roberta.Client
~/.dotnet/dotnet publish -c Release -r linux-arm64 -o ~/roberta/client --self-contained false -p:PublishSingleFile=true

echo -e "\e[32mConfiguring Roberta Client service...\e[0m"
sudo cp ~/uv-roberta/src/Roberta.Client/Roberta.Client/robcli.service /etc/systemd/system/robcli.service
sudo systemctl enable robcli.service


echo -e "\e[32mBuilding CPU Monitor...\e[0m"
cd ~/uv-roberta/src/CpuMonitor
g++ -o ~/roberta/cpu_monitor cpu_monitor_main.cpp
cd ~

echo -e "\e[32mBuilding RX Reader...\e[0m"
cd ~/uv-roberta/src/Roberta.RxReader
g++ -o ~/roberta/rx_reader_main rx_reader_main.cpp rx_reader.cpp -lpigpio


echo -e "\e[32mRebooting...\e[0m"
sudo reboot now
