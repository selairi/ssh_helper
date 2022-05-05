# Download Ubuntu dependencies
sudo apt install g++ cmake qtbase5-dev pkg-config qtbase5-dev-tools libssh-dev qterminal

# Compiling
cd build
cmake ..
make -j4

# Installing
sudo make install
