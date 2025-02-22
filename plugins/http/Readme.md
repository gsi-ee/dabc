\page http_plugin HTTP plugin for DABC (libDabcHttp.so)

\ingroup dabc_plugins

\subpage http_readme


\page http_readme Use of HTTP plugin

## Introduction
HTTP server implementation in DABC based on civetweb project.
Following classes are implemented:
   - http::Server
   - http::FastCgi
   - http::Civetweb


## Compilation
Compiled automatically during DABC compilation.


## Starting from xml file
First of all, libDabcHttp.so should be loaded.
Minimal configuration for server looks like:

~~~~~
    <HttpServer name="http" port="8090"/>
~~~~~

Following parameters can be specified:

| Name        | Description |
| --------:   | :---------- |
| port        | HTTP port number (default 8090) |
| auth_file   | name of authentification file, where passwords in htdigest format saved |
| auth_domain | domain name used for authentification |
| auth_default| is authentication per default required or not |
| ports       | HTTPS port numbver (default none) |
| ssl_certif  | file name with SLL certificate |


## Authentification
For authentification htdigest file format is used. To create such file,
following shell command should be executed:

    [shell] htdigest -c .htdigest dabc@server user

Where "dabc@server" is authentification domain, which should be later specified as
auth_domain parameter in xml file. ".htdigest" file can contain many defined users.

If authentification is configured, user will be requested to input account name
and password when first time accessing http server.

When using command line tools (like wget or curl), following arguments should be specified:

    [shell] curl --user "user:passwd" http://server:8090/MBS/X86L-9/CmdMbs/execute?cmd=show --digest -o some.txt

    [shell] wget --http-user=user --http-passwd=passwd http://server:8090/MBS/X86L-9/CmdMbs/execute?cmd=show -O some.txt

When creating hierarchies, for each element or folder '_auth' property can be specified,
which decides if authentication is required or not for that element.
