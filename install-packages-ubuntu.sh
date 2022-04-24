# Download Ubuntu dependencies
apt install g++ cmake qt5-default qtbase5-dev-tools libssh-dev qterminal

# Compiling
cd build
cmake ..
make -j4

# Installing
sudo make install
