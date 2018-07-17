sudo examples/teotru_load teo-tru-ser server -p 9700 -d && examples/teotru_load teo-tru-cli teo-tru-ser -a localhost -r 9700
echo "Result:" $?
sudo killall lt-teotru_load