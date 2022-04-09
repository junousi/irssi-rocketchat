# irssi-rocketchat

irssi-rocketchat is an irssi module to connect to a [Rocket.Chat] instance

It is still in a work-in-progress state and a lot of things does not work, so
use it at your own risks.

## Why ?

Because I like irssi and I don't like using a mouse for a text chat, nor
waiting 2 seconds to autocomplete a nick

## What works ?

* login using an authentication token (you need to log in using a web browser
  to generate a token)
* list public channels
* list users
* join an existing room (public/private channel, discussion, direct message).
  leaving a room is not implemented
* create direct message using `/query <nick>`
* receive/send messages from/to rooms
* automatic join to rooms when a message is received
* room history: the last 10 messages can be loaded using `/rocketchat history`
* nicklist

## Installation

### Requirements

* [CMake](https://cmake.org)
* A C compiler toolchain
* irssi development files
* [glib](https://developer.gnome.org/glib/)
* [Jansson](https://digip.org/jansson/) (>= 2.11)
* [libwebsockets](https://libwebsockets.org/) (>= 4.1) with glib support

For most of these you can use your package manager:

```sh
apt-get install cmake build-essential irssi-dev libglib2.0-dev libjansson-dev
```

But you will need to build libwebsockets from source. Compiling libwebsockets
requires libssl development files

```sh
apt-get install libssl-dev
```

```sh
git clone -b v4.1-stable https://libwebsockets.org/repo/libwebsockets
cd libwebsockets
mkdir build && cd build
cmake -DLWS_WITH_GLIB=1 .. && make
# as root
make install
ldconfig
```

### Building

Once all dependencies are installed, build and install irssi-rocketchat

```sh
cd /path/to/irssi-rocketchat
mkdir build && cd build
cmake .. && make && make install
```

This will install irssi-rocketchat in ~/.irssi/modules

## Usage

First generate an access token:

* sign in with a web browser to your Rocket.Chat instance
* on your account page, go to "Personal access tokens"
* create a new access token and keep it visible, you will need to copy it into
  your irssi config file

Then define a new chatnet and a new server in your ~/.irssi/config

```
chatnets = {
    MyRocketChat = {
        type = "rocketchat";
    }
};
servers = (
    {
        address = "chat.example.com";
        chatnet = "MyRocketChat";
        port = "443";
        use_tls = "yes";
        password "<your-access-token>";
    }
);
```

Then open irssi and enter the following commands:

```
/load rocketchat
/connect MyRocketChat
```

Once connected you can list public channels

```
/rocketchat channels
```

```
foo (ID: aQhQEXsMuoS439dGS)
general (ID: GENERAL)
test (ID: 497BsckNzoaT2PeMF)
```

and join one or more using their ID or their name

```
/join aQhQEXsMuoS439dGS,GENERAL,test
```

To automatically join a room when connected to the server you can add it to
your channels list

```
/channel add -auto test MyRocketChat
```

To start a private discussion with someone, use the `/query` command

```
/query john
```

## Message format & theme

Message formats are customizable in the theme

Type `/format rocketchat` to get the list of all available formats

By default, these formats use 2 "abstracts" that are not defined in irssi's
default theme: `msgid` and `tmid`. You need to define them:
1. open your theme file (by default it's `~/.irssi/default.theme`),
2. find the `abstracts` section,
3. inside this section add the following lines (feel free to customize, see
   https://github.com/irssi/irssi/blob/master/docs/formats.txt):
```
msgid = "[$0]"; # message id
tmid = "[$0]"; # thread id
```

## Commands

### `/rocketchat channels`

List public channels

### `/rocketchat users`

List users

### `/rocketchat history`

Print the last 10 lines of history for the current channel/query

### `/rocketchat reply <tmid> <message>`

Send a message to a thread.

`<tmid>` is the thread id. It's displayed before each received message that is
from a thread. You can also reply to any message to create a new thread.


## Known bugs

* `/query <nick> <message>` only works when the query already exists. If the
  query does not exist yet the message will not be sent.

[Rocket.Chat]: https://rocket.chat/
