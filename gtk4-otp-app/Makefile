CFLAGS = `pkg-config --cflags gtk4 libadwaita-1` -I/usr/include/sodium
LIBS = `pkg-config --libs gtk4 libadwaita-1` -lsodium

app: main.c
	gcc $(CFLAGS) -o otp-app main.c $(LIBS)

clean:
	rm -f otp-app 