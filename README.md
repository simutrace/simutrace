![Logo](/simutrace/documentation/theme/Simutrace.png)

Simutrace
=========

Simutrace is a tracing framework for full system simulators and has been
conceived with full length, no-loss tracing of high-frequency events such as
executed CPU instructions and main memory references in mind. 

The framework places no restriction on the type and number of captured events 
and employs a scalable storage format, which easily handles traces of hundreds 
of gigabytes in size. Simutrace has been particularly extended to facilitate 
efficient memory tracing by incorporating aggressive, but fast memory trace 
compression.

Simutrace is a research project of the operating system group at Karlsruhe 
Institute of Technology (KIT).


Table of Contents
-----------------

  - [Architecture](#architecture) 
  - [Getting Started](#getting-started)
    - [Supported Platforms](#supported-platforms)
    - [Installation](#installation)
    - [Building from Source](#building-from-source)
    - [Starting Simutrace](#starting-simutrace)
    - [Writing and Reading Traces](#writing-and-reading-traces)
  - [Documentation](#documentation)
    - [Buiding the Documentation](#building-the-documentation) 
  - [Bugs and Feature Requests](#bugs-and-feature-requests)
    - [Known Issues](#known-issues)
  - [Versioning](#versioning)
  - [Contributing](#contributing)
  - [Copyright and Licenses](#copyright-and-licenses)
  - [3rd Party Components](#3rd-party-components)
  - [Authors](#authors)


Architecture
---------------------------

Simutrace uses a client-server architecture, where a client such as an
extension in a full system simulator collects events (e.g., memory accesses)
and a storage server receives the data, performs trace compression and storage
as well as provides easy access to the data for later analysis and inspection.
The server can be situated on the same as well as on a remote host. 

When connecting to the server, the client is assigned a session and may create
a new or open an existing trace data store. Within a store, trace events are 
organized into streams. Each stream stores events of a single type (e.g., 
memory writes). A client may create an arbitrary number of streams using 
built-in or custom event types. Writing and reading trace data with Simutrace
thus follows common streaming semantics.

For a thorough introduction to Simutrace and its capabilities please refer to 
our white paper [Efficient Full System Memory Tracing with Simutrace](about:blank).

#### Binary Files

Simutrace consists of three libraries and one executable:

<b>libsimutrace(.so/.dll)</b>: This is the client library offering the public 
C interface to Simutrace. This library needs to be used by all applications
that want to use Simutrace. See http://simutrace.org/documentation for a 
documentation of the API.

<b>simustore(.exe)</b>: The server executable, containing all trace processing
and storage functionality.

<b>libsimubase, libsimustore</b>: These are static libraries that encapsulate
common code for the client library and the storage server. You won't see these
as build output as they are statically linked into the client library and
storage server binary, respectively.


Getting Started
---------------

In this section, we first describe the supported platforms, prerequisites and
the environment for building Simutrace from source. You may download 
pre-compiled binaries at http://simutrace.org/downloads. The section continues
by giving a short introduction on how to start Simutrace.

### Supported Platforms

Simutrace supports Windows Vista+ and Linux. Since trace processing is a
memory intensive task, we only support 64 bit operating system versions.
We tested Simutrace on x86-64 hardware, only.

For best performance, we recommend running the server on a system with at least
8 CPU cores (4+4) and 12 GiB of RAM. The server very well operates a 24 core 
dual-socket Xeon system at full load when reading a memory trace with 
700 MiB/s. The hardware requirements for the client are significantly lower as
all heavyweight processing is done in the server process. The client runs
perfectly fine on a single core with 2 to 4 GiB of RAM. When running the 
client and server on the same machine, the memory of the client is shared with
the server and does not add to the server's requirements.

### Installation

This section describes the installation of one of the pre-compiled versions of 
Simutrace and its prerequisites. If you cannot find a version of Simutrace on
http://simutrace.org/downloads suitable to your environment (e.g., OS), you can
build Simutrace [from source](#building-from-source).

Note: The installation does not include an extension for a full system 
simulator to generate trace events from a simulation. For information on 
simulator extensions, please refer to http://simutrace.org.

#### Step 1: libconfig

Simutrace utilizes [libconfig](http://www.hyperrealm.com/libconfig/) for
configuration file parsing. To run Simutrace you first have to install
libconfig (version 1.4.9 or compatible, c++ binding). On <b>Windows</b>, we 
provide the library together with the binary installation package. 
On <b>Linux</b>, you probably find a binary package within your distribution's
package management system. On Ubuntu Linux 14.04 LTS you can install libconfig
by entering:

    $ sudo apt-get install libconfig++9

#### Step 2: Visual C++ Redistributable Package 2013 (Windows-only)

The Visual C++ Redistributable package installs run-time components that are 
required to run Simutrace. Download the package from Microsoft and follow the
instructions given on the download page.

<i>If you have Visual Studio 2013 installed, the required components are 
already on your system.</i>

#### Step 3: Simutrace

Download the latest version of Simutrace from http://simutrace.org/downloads.

### Building from Source

If you want to make changes to Simutrace or want to run Simutrace on a platform
that is not officially supported, you can build the software from source.

#### Step 1: Setting up the Build Environment

On <b>Windows</b>, you need Microsoft Visual Studio 2013 or newer with
Visual C++ to build Simutrace. A free (express) version can be downloaded from 
Microsoft. Dependencies on libraries are configured as NuGet packages and are 
resolved by Visual Studio before the first build automatically.

On <b>Linux</b>, you have to install the basic development tools such a C++11
compatible GCC (version 4.7 or higher) and make. Some distributions provide a 
package that will install the required tools. On Ubuntu Linux 14.04 LTS you 
can enter:

    $ sudo apt-get install build-essential

You further need to install the development version of 
[libconfig](http://www.hyperrealm.com/libconfig/) (version 1.4.9 or compatible,
c++ binding).

    $ sudo apt-get install libconfig++-dev

#### Step 2: Cloning the Source Repository

Before you can clone the source repository you need to install git. On
<b>Windows</b> you may install it from https://msysgit.github.io/. On 
<b>Linux</b> you will most probably find a binary package in your system's
package management system. On Ubuntu 14.04 LTS you can enter:

    $ sudo apt-get install git  

After installing git, you clone the source repository by entering:

    $ git clone https://github.com/simutrace/simutrace.git

The repository encompasses the server component, the C client interface
library to write simulator extensions and analysis clients, sample code and 
documentation sources.

The repository has the following structure:

<pre>
.
|-- <b>samples</b>
|   `-- simpleclient        <i>C sample client application, which writes</i>
|                           <i>and reads data to/from a trace.</i>
`-- <b>simutrace</b>
    |-- documentation       <i>Documentation source and theme.</i>
    |-- <b>include</b>             <i>Header files, use SimuTrace.h.</i>
    |   |-- simubase        <i>Internal headers for libsimubase.</i>
    |   `-- simustor        <i>Internal headers for libsimustor.</i>
    |-- libsimubase         <i>Internal platform abstraction library.</i>
    |-- libsimustor         <i>Internal tracing core library.</i>
    |-- <b>libsimutrace</b>        <i>Public client library.</i>
    `-- <b>storageserver</b>       <i>Trace storage and access server.</i>
        `-- simtrace        <i>simtrace (*.sim) storage format provider</i>
            |               <i>and type-specific trace encoders/decoders.</i>
            `-- VPC4        <i>Prediction-based memory trace compressor.</i>
</pre>

#### Step 3: Building

To build the code on <b>Windows</b> you open the solution file 
(<i>SimuTrace.sln</i>) in the root of the source tree and choose 
BUILD | BUILD SOLUTION in Visual Studio.

On <b>Linux</b>, you go into the root directory of the source tree and run
make:

    $ make

The final executables will be located in <i>build/Release</i> or 
<i>build/Release-Windows</i>, respectively, depending on your platform.

##### Debug Version
To build the debug version of Simutrace you have to choose the
appropriate solution configuration in Visual Studio (<i>Debug-Windows</i>) or
on Linux type:

    $ make CONFIG=DEBUG 

### Starting Simutrace

Simutrace uses a client-server architecture. Starting Simutrace thus 
constitutes starting the server on the one side and starting one or more
clients on the other side.

You start the <b>server</b> by running the main <i>simustore(.exe)</i> 
executable. The server will then wait for clients to connect.

The server can take various settings to change the way it operates
and to regulate the amount of resources it allocates. We recommend downloading 
the sample configuration file from our website, which contains a detailed 
description of the settings available and their default values. When you build 
Simutrace from source, the configuration file is already in the output 
directory. To supply the file during startup, type:

    $ ./simustore -c simustore.cfg

If you start Simutrace from Visual Studio, the configuration file is supplied
by default.

How to start the <b>client</b>, depends on the client application. A client may
be an extension in a full system simulator, which connects to the server and
outputs all events it observes in the simulation. A client may also be an
analysis software, which reads the traces back in to perform some form of 
analysis.

Simutrace does not come with any specific client application, yet. Instead we
provide samples that show you how to bind against the client library
(<i>libsimutrace(.so/.dll)</i>) and use the Simutrace API to work with the
storage server. All samples can be started by just running the corresponding
executable without any further arguments.

### Writing and Reading Traces

To write or read a trace you must develop a client application that calls
the server as desired. The client library provides a C interface for this
purpose. You may use the interface by including <i>SimuTrace.h</i>.

Note: if you wish to use a different programming language for your client,
see if and how your preferred language can bind to a native C shared/dynamic
library.

Please see the source of the sample clients as well as the documentation for 
details on how to bind to the client library and invoke the API.


Documentation
-------------

Unfortunately, the internal classes, interfaces and components of Simutrace are
not documented, yet. However, you will find a complete documentation of the
public client API, required to write or read traces, at
http://simutrace.org/.

### Building the Documentation

The documentation in Simutrace is automatically build from source using
doxygen. On <b>Windows</b>, you will find the latest version of doxygen at
http://www.doxygen.org. You are then able to execute <i>make-doc.bat</i>
in the root of the source tree.

On <b>Linux</b>, install doxygen with your package management system. 
On Ubuntu 14.04 LTS you can enter:

    $ sudo apt-get install doxygen

Afterwards, you can build the documentation by typing in the root of the
source directory:

    $ make documentation

You will find the documentation in <i>build/documentation/html</i>.


Bugs and Feature Requests
-------------------------

A bug is a <i>demonstrable problem</i> that is caused by code in the 
repository. Although we give our best to provide tools that simply work, bugs 
always happen.

Guidelines for bug reports:

  - <b>Use the GitHub issue search</b> - check if the issue has already been 
    reported.

  - <b>Check if the issue has been fixed</b> - try to reproduce it using the 
    latest release or development branch in the repository.

  - <b>Isolate the problem</b> - ideally create a reduced test case and a live 
    example.

A good bug report shouldn't leave others needing to chase you up for more 
information. Please try to be as detailed as possible in your report. 
What is your environment? What steps will reproduce the issue? 
On which operating system do you experience the problem? Does the bug happen
on other operating systems, too? What would you expect to be the outcome? 
All these details will help to fix potential bugs.

Example:

<pre>
Short and descriptive example bug report title

A summary of the issue and the environment in which it occurs. If suitable, 
include the steps required to reproduce the bug.

    + This is the first step
    + This is the second step
    + Further steps, etc.

<url> - a link to the reduced test case

Any other information you want to share that is relevant to the issue being
reported. This might include the lines of code that you have identified as
causing the bug, and potential solutions (and your opinions on their 
merits).
</pre>

To report an issue please use the 
[issue tracker](https://github.com/simutrace/simutrace/issues).

### Feature Requests

Since the project is primarily maintained by a single person, in most cases we 
won't be able to implement requested features. Nevertheless, we would be happy
to know where Simutrace does not fulfill your requirements and from what 
improvements you would benefit from.

Please use the [issue tracker](https://github.com/simutrace/simutrace/issues)
for feature requests.

### Known Issues

For known issues, please see [KNOWN ISSUES](/KNOWNISSUES).


Versioning
----------

Simutrace employs the semantic versioning scheme (http://semver.org/). For the
current version, please refer to [VERSION](/VERSION).


Contributing
------------

Simutrace has been designed with extensibility in mind. It is particularly easy
to add support for other processing logic (e.g., for instruction flow encoding)
or trace formats. Being able to add further communication channels, such as 
RDMA-based networks, has always been intended in the design, too. Platform
dependent code is encapsulated in <i>libsimubase</i>, facilitating ports of
Simutrace to operating systems other than Windows and Linux. Last but not 
least, the client-server architecture makes developing trace sources (e.g., 
plug-ins for full system simulators) and analysis software straightforward.

If you want to contribute an extension to Simutrace, if you have developed a 
trace source or analysis client that could be of interest to others or if you 
have improved any of the existing components, please use the 
[issue tracker](https://github.com/simutrace/simutrace/issues) as preferred 
channel. 

Before submitting extensions to Simutrace, please carefully study the
existing source. <b>Your code must adhere to the same coding style to be 
accepted in the repository (formatting, exception handling, etc.)</b>.

We would be happy to see your contribution in the next release!

If you have found a <b>bug</b> or have a <b>feature request</b>, please see 
[Bugs and Feature Requests](#bugs-and-feature-requests).


Copyright and Licenses
----------------------

Copyright 2014 (C) Karlsruhe Institute of Technology (KIT)</br>
[Marc Rittinghaus](http://os.itec.kit.edu/rittinghaus), 
[Thorsten Groeninger](http://os.itec.kit.edu/english/21_2748.php)

Simutrace is licensed under the [GNU General Public License](/LICENSE-GPL) 
and the [GNU Lesser General Public License](/LICENSE-LGPL), depending on the
component. Each source file includes a reference to its license in the 
header, leading to the following licensing model:

| Component     | License |
|:--------------|--------:|
| libsimubase   | LGPL    |
| libsimustor   | LGPL    |
| libsimutrace  | LGPL    |
| storageserver | GPL     |


3rd Party Components
--------------------

Simutrace makes use of the following 3rd party libraries, components or
algorithms:
  - [libconfig](http://www.hyperrealm.com/libconfig/)
  - [ezOptionParser](http://ezoptionparser.sourceforge.net/)
  - [MurmurHash3](https://code.google.com/p/smhasher/wiki/MurmurHash)
  - [7zip LZMA Compressor](http://www.7-zip.org/)

Simutrace's memory trace compressor uses a modified version of the algorithm
presented in <i>M. Burtscher et al. The vpc trace-compression algorithms. 
Computers, IEEE Transactions on, 54(11):1329–1344, 2005.</i>

The debug build of Simutrace includes the 
[FastDelegate](http://www.codeproject.com/cpp/FastDelegate.asp) library.


Authors
-------

Simutrace has been primarily developed and is maintained by
[Marc Rittinghaus](http://os.itec.kit.edu/rittinghaus) 
< <rittinghaus@kit.edu> > as part of his research at Karlsruhe Institute of 
Technology (KIT) in the area of full system tracing and acceleration of
full system simulation.

Countless hours of invaluable prototyping and 
testing have been contributed by 
[Thorsten Groeninger](http://os.itec.kit.edu/english/21_2748.php), Karlsruhe 
Institute of Technology (KIT).