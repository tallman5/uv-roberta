#!/bin/sh

cd ~

echo -e "\e[32mUpdating...\e[0m"
sudo apt update


echo -e "\e[32mUpgrading...\e[0m"
sudo apt upgrade -y


# echo -e "\e[32mInstalling cmake...\e[0m"
# wget https://cmake.org/files/v3.26/cmake-3.26.4-linux-aarch64.tar.gz
# tar -xvf cmake-3.26.4-linux-aarch64.tar.gz
# rm -rf cmake-3.26.4-linux-aarch64.tar.gz
# cd ~


# echo -e "\e[32mInstalling ninja...\e[0m"
# sudo apt install ninja-build -y


echo -e "\e[32mInstalling git...\e[0m"
sudo apt install -y git


echo -e "\e[32mInstalling FTP...\e[0m"
sudo apt install -y vsftpd
startPath="/etc/vsftpd.conf"
tempPath="/etc/vsftpd.temp"
cp "$startPath" "$tempPath"
sed -i 's/#write_enable=YES/write_enable=YES/' "$tempPath"
mv "$tempPath" "$startPath"
sudo service vsftpd restart


echo -e "\e[32mInstalling pigpio...\e[0m"
sudo apt install -y pigpio
# Uncomment next line if running pigpio from Python
# sudo apt install -y python-pigpio python3-pigpio


echo -e "\e[32mInstalling .NET...\e[0m"
curl -sSL https://dot.net/v1/dotnet-install.sh | bash /dev/stdin --channel 6.0.4xx


echo -e "\e[32mUpdating .bashrc...\e[0m"
echo '' >> ~/.bashrc
echo 'export DOTNET_ROOT=$HOME/.dotnet' >> ~/.bashrc
echo 'export PATH=$PATH:$HOME/.dotnet' >> ~/.bashrc
echo 'export ASPNETCORE_URLS="http://*:5000"' >> ~/.bashrc
# echo 'export PATH=$PATH:$HOME/cmake-3.26.4-linux-aarch64/bin' >> ~/.bashrc
source ~/.bashrc


echo -e "\e[32mInstalling ustreamer...\e[0m"
sudo apt install -y libjpeg-dev libevent-dev libbsd-dev nlohmann-json3-dev libwebsockets-dev
git clone --depth=1 https://github.com/pikvm/ustreamer
cd ustreamer/
make
cd ~
