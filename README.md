# rum-jamajka-installer
A Linux systems installer built for a future distribution: RumJamajkaOS

# Building
Clone rum-jamajka-installer from GitHub, run CMake, and compile it:
```
$ git clone https://github.com/calamares/calamares.git
$ mkdir calamares/build
$ cd calamares/build
$ cmake -DCMAKE_BUILD_TYPE=Debug ..
$ make
```

Run it straight from the `build` directory in
one of the following ways:
```
$ ./calamares -d
$ sudo ./calamares -d
$ pkexec ./calamares -d
```

Copy `settings.conf`
into the build-directory:
```
$ cp ../settings.conf .
```
