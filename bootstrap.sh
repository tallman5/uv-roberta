#!/bin/sh


cd ~


echo -e "\e[32mUpdating...\e[0m"
sudo apt update


echo -e "\e[32mUpgrading...\e[0m"
sudo apt upgrade -y


echo -e "\e[32mFix for missing /dev/serial/by-id...\e[0m"
sudo cp /usr/lib/udev/rules.d/60-serial.rules /usr/lib/udev/rules.d/60-serial.old
sudo wget -O /usr/lib/udev/rules.d/60-serial.rules https://raw.githubusercontent.com/systemd/systemd/main/rules.d/60-serial.rules


echo -e "\e[32mInstalling git...\e[0m"
sudo apt install -y git


echo -e "\e[32mInstalling FTP...\e[0m"
sudo apt install -y vsftpd
sudo cp /etc/vsftpd.conf /etc/vsftpd.old
sudo sed -i 's/#write_enable=YES/write_enable=YES/' '/etc/vsftpd.conf'
sudo service vsftpd restart


echo -e "\e[32mInstalling pigpio...\e[0m"
sudo apt install -y pigpio


echo -e "\e[32mInstalling .NET...\e[0m"
curl -sSL https://dot.net/v1/dotnet-install.sh | bash /dev/stdin --channel 6.0.4xx


echo -e "\e[32mUpdating .bashrc...\e[0m"
echo '' >> ~/.bashrc
echo 'export DOTNET_ROOT=$HOME/.dotnet' >> ~/.bashrc
echo 'export PATH=$PATH:$HOME/.dotnet' >> ~/.bashrc
echo 'export ASPNETCORE_URLS="http://*:5000"' >> ~/.bashrc
source ~/.bashrc


echo -e "\e[32mInstalling ustreamer...\e[0m"
sudo apt install -y libjpeg-dev libevent-dev libbsd-dev nlohmann-json3-dev libwebsockets-dev
git clone --depth=1 https://github.com/pikvm/ustreamer
cd ustreamer/
make
cd ~


echo -e "\e[32mCreating directories...\e[0m"
mkdir robhub
mkdir robgps


echo -e "\e[32mRebooting...\e[0m"
sudo reboot now
