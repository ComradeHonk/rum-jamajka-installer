# rum-jamajka-installer
A Linux systems installer built for a future distribution: RumJamajkaOS

# Building
Clone rum-jamajka-installer from GitHub, run CMake, and compile it:
```
$ git clone https://github.com/ComradeHonk/rum-jamajka-installer.git
$ mkdir rum-jamajka-installer/build
$ cd rum-jamajka-installer/build
$ cmake -DCMAKE_BUILD_TYPE=Debug ..
$ make
```

Copy `settings.conf`
into the build-directory:
```
$ cp ../settings.conf .
```

Run it straight from the `build` directory in
one of the following ways:
```
$ ./calamares -d
$ sudo ./calamares -d
$ pkexec ./calamares -d
```

**NOTE**: rji is not yet functional, which means it won't install anything, so it's relatively safe to test, but you still might encouter bugs.
Oh, and also rji needs certain build dependencies, but because this repo is a one time thing I'm too lazy to put them here ;)
