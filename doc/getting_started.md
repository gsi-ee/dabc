# Getting started with DABC {#dabc_getting_started}

## Downloading DABC

Most proper way to download DABC is to check out head version from repository.

    [shell] svn co https://subversion.gsi.de/dabc/trunk dabc

Alternative, one could check out latest release tag:

    [shell] svn co https://subversion.gsi.de/dabc/tags/206-00 dabc20600


## Compile DABC

### cmake

Create build directory and call `cmake` and then:

    [shell] mkdir build
    [shell] cd build
    [shell] cmake <path/to/dabc>
    [shell] make -j

When calling `cmake`, one can enable or disable following plugins:

| Name        | Default | Description |
| --------:   | ------: | :---------- |
| aqua | ON | AQUA plugin |
| bnet | ON | BNET plugin |
| dim | OFF | DIM plugin |
| ezca | OFF |  EZCA (EPICS) plugin |
| fesa | OFF |  FESA plugin |
| gosip | OFF |  GOSIP plugin |
| hadaq | ON |  HADAQ plugin |
| http | ON |  HTTP plugin |
| ltsm | OFF |  LTSM plugin |
| mbs | ON |  LTSM plugin |
| rfio | ON |  RFIO plugin |
| saft | OFF |  SAFT plugin |
| stream | ON |  Stream plugin |
| user | ON |  USER plugin |
| verbs | ON |  VERBS plugin |
| root | ON | ROOT plugin |
| mbsroot | ON | MBS-ROOT plugin |

Like:

    [shell] cmake -Dverbs=OFF -Ddim=ON <path/to/dabc>



### make


To compile DABC, in most cases it is enough to call `make` in checkout directory.

    [shell] make -j4

It is recommended to use -jN option to speedup compilation.

DABC includes [OFED VERBS](http://www.openfabrics.org) support,
which is not always compiles smoothely on all platforms.
To disable VERBS compilation, one could use:

    [shell] make -j4 noverbs=1



## Running DABC

When DABC successfully compiled, dabclogin script is created.
Script must be called before DABC can be used:

    [shell] . your_dabc_path/dabclogin

If necessary, dabclogin script could be copied to any other location,
which is availible in PATH directory (for instance, `~/bin`). In that case one could do:

    [shell] . dabclogin

To run DABC, xml configuration file should be created. There are different files.
Some of them could be found in `$DABCSYS/applications` directory. For instance:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.xml}
<?xml version="1.0"?>
<dabc version="2">
  <Context name="core-test">
    <Run>
      <lib value="libDabcCoreTest.so"/>
      <runfunc value="RunCoreTest"/>
      <logfile value="core-test.log"/>
      <runtime value="100"/>
    </Run>
  </Context>
</dabc>
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

To run single-node application, one should call:

    [shell] dabc_exe your_file.xml

Running application can be always normally stopped by Ctrl-C keys combination.

If XML files contains more than one application, one should use `dabc_run` script:

    [shell] dabc_run your_file.xml run

