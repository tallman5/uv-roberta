#!/bin/sh

cd ~/uv-roberta
git pull

cd~
echo -e "\e[32mStopping services...\e[0m"
sudo systemctl stop robhub.service
sudo systemctl stop robcli.service


echo -e "\e[32mBuilding Hub...\e[0m"
cd ~/uv-roberta/src/Roberta.Hub/Roberta.Hub
dotnet publish -c Release -r linux-arm64 -o ~/roberta/hub --self-contained false -p:PublishSingleFile=true


echo -e "\e[32mBuilding Client...\e[0m"
cd ~/uv-roberta/src/Roberta.Client/Roberta.Client
dotnet publish -c Release -r linux-arm64 -o ~/roberta/client --self-contained false -p:PublishSingleFile=true


cd~
echo -e "\e[32mStarting services...\e[0m"
sudo systemctl start robhub.service
sudo systemctl start robcli.service


echo -e "\e[32mChecking services...\e[0m"
sudo systemctl status robhub.service
sudo systemctl status robcli.service
