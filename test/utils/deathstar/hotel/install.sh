sudo apt update
sudo apt-get install -y docker.io docker-compose
sudo apt install -y luarocks
sudo luarocks install luasocket

git clone --depth=1 https://github.com/delimitrou/DeathStarBench.git
cd DeathStarBench/hotelReservation
sudo docker-compose up -d


