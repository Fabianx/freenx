#gcc -g -o network network.c `gtk-config --cflags --libs`
gcc -g -o network network.c `pkg-config --cflags --libs gtk+-2.0`