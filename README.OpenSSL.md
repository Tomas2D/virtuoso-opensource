Virtuoso and OpenSSL
====================

*Copyright (C) 1998-2022 OpenLink Software <vos.admin@openlinksw.com>*

Virtuoso Open Source Edition uses the OpenSSL libraries for cryptographic operations as well as
handling both client-side and server-side secure connections for both HTTPS as well as iSQL/ODBC
transport layers.

Virtuoso can be compiled against OpenSSL 0.9.8i up to and including OpenSSL v3.0.x, which is the
current Long Term Support (LTS) release of OpenSSL.

Some operating systems like Mac OS X or Windows do not supply OpenSSL at all, but require third
party ports in order to work.

During configure time Virtuoso will check the availability and version of the installed OpenSSL
development headers and libraries on your system and will report any issues it finds.


Mac OS X
--------
As Apple is actively deprecating OpenSSL from Mac OS X, your system either has a pretty old version
of openssl, and in case of High Sierra (10.13) Apple removed the required include files from the
/usr/include/openssl directory completely.

We recommend installing the OpenSSL 1.0.2 library using:
```
    $ brew install openssl
```

And at configure time you can use the following:
```
    $ sh ./configure \
      ..... \
      ..... \
      --enable-openssl=/usr/local/opt/openssl/
```

More information on porting VOS on Mac OS X can be found in [README.MACOSX.md](README.MACOSX.md)


Ubuntu 22.04 LTS
----------------
Ubuntu 22.04 ships with OpenSSL 3.0.2.

As of VOS 7.2.7 VOS has been upgraded to work with that version.

To install the OpenSSL 3.0.2 SDK you can use the following command:
```
    $ sudo apt-get install libssl-dev
```


Ubuntu 18.04 LTS
----------------
Ubuntu installs both an OpenSSL v1.0.2 runtime as well as an OpenSSL v1.1.0 runtime in the form
of shared libraries which are installed side-by-side on the system.

A developer can choose to install either the 1.0 or the 1.1 software development kit (SDK) which
includes the necessary header files and development libraries.

As the current VOS release is compatible with OpenSSL 1.1.x you can use the following command
to install the OpenSSL development SDK.
```
    $ sudo apt-get install libssl-dev
```

Debian 9
--------
Debian uses the same package names as Ubuntu:
```
    $ sudo apt-get install libssl-dev
```

Fedora 28
---------
Fedora uses:
```
     $ sudo dnf install --allowerasign openssl-devel
```


Windows
-------
The OpenSSL library needs to be built as a static library using Visual Studio.
The detailed instructions for building OpenSSL can be found in the INSTALL.W32
document in the OpenSSL source distribution.

**IMPORTANT**: By default the OpenSSL library is built using MSVCRT compile flags,
leading to conflicts when linking the Virtuoso Open-Source binaries. To resolve
the conflicts, after unpacking the OpenSSL source tarball, you need to edit the
/util/pl/VC-32.pl and change the "cflags" to use the "/MT" and "/MTd" compiler
switches instead of the "/MD" and "/MDd".

Finally the libeay32.lib and ssleay32.lib from /out32 and files from /inc32/
openssl must be copied to the <Virtuoso Open Source dir>/win32/openssl/

More notes on porting Virtuoso on Windows can be found in [README.WINDOWS](README.WINDOWS).


Custom installation
-------------------
If you have performed a custom build of OpenSSL on your own system and/or if OpenSSL is installed
in a non-standard location, you can use the following flag during configure to point the build
system to the location where your OpenSSL headers and libraries are installed:
```
    $ sh ./configure \
      ..... \
      ..... \
      --enable-openssl=/opt/openssl/
```

The configure script will check /opt/openssl/include for the required header files and
/opt/openssl/lib for the libraries, before any standard locations embedded in the compiler.
