sudo apt update
sudo apt install -y docker-io
sudo apt install -y docker-compose
sudo apt install -y python3 python3-pip
sudo apt install libssl-dev
sudo apt install libz-dev
sudo apt install luarocks
sudo luarocks install luasocket


git clone --depth=1 https://github.com/delimitrou/DeathStarBench.git
cd DeathstarBench/mediaMicroservices
docker-compose up -d
