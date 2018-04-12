# Commune

Commune is a protocol for peer-to-peer chat on a network.

Inspired by the classic Numb3rs clip where IRC is described as like ships passing in the night,
Commune is intended for unmoderated communication between interested parties.

Compared to IRC it's totally decentralized, not just federated. There are no servers and no rendezvous points. There are no reserved nicknames. Peers may be identified just by their IP address if desired.

Compared with link-local XMPP, it's intended to be much simpler. A smaller set of message types, no need for XML
parsing, and instead of Multicast DNS for peer discovery, peers simply join the channel of their choice.

Downsides are that this works best with link-local multicast, not for global messaging where unicast, proxying/bridging & rendezvous points may be required.


## Protocol

All communication is done using a single IPv6 link-local address. IPv4 support might be added later.

The link-local address is currently ff02::114, which is an address intended for "any private experiment". If the protocol proves to be useful, a new address will be requested from IANA.

Ports 0-1023 are generally unused to avoid requiring access to privileged ports.

All multi-byte numeric fields are big-endian. All strings are UTF-8.


### Protocol format

1 byte: version = 0x00
4 bytes: message type
The rest of the packet is the message body.

Messages must, of course, fit within a single UDP packet on the link.

Unicast messages are sent to port 1818. Both multicast & unicast messages SHOULD be sent from port 1818.

Port 1818 on the multicast address is reserved and SHALL NOT be used as a channel.

Message type is conventionally a 4-character ASCII string.

Message types:

MESG: A text message, UTF-8 encoded. Multicast or unicast. Unicast for one-to-one messages.

NICK: A nickname representing the user. Multicast or unicast. May be sent in response to a QNIK query or at any time a peer wishes to update their name with other peers. An empty name indicates that they would prefer to be identified only by their netID. Nicknames may be truncated when displayed by user agents. Due to the nature of Unicode and languages, there is no protocol-specified limit on the octet-length or display-length of a nickname, other than the maximum packet size. Unsolicited NICK messages (ie. those that are not in response to a QNIK query) MUST only be sent using multicast to the user's channels. (In particular, a user can elect to have a different nickname in each channel, if desired.)

QNIK: Nickname query. This can be used to get a list of users in a channel, or to get a single peer's nickname. Responses are always multicast to avoid duplicate unicast traffic.
a) list of users: QNIK multicast to channel with an empty message body. Channel members respond NICK via multicast. Channel members may elect to remain "hidden" by not responding.
b) Individual name: Sent multicast to channel with the IP address of the peer in the message body. Requests that the peer indicate its nickname to the channel via a multicast NICK response to the channel.

GBYE: Leaving a channel. No message body. Multicast to the channel. Sent immediately before leaving the channel (multicast group) to allow the peer to be removed from the user-list. This is optional. User agents may elect to automatically remove users from the user list after some period of time, or not to show a user list at all. This is especially useful on populous channels where it may be helpful to only show users who have sent a MESG recently.


### Protocol operation

Each port on the multicast address is a "channel" in the IRC sense. Channels may be assigned names locally. There is no "topic" or even shared idea of what the "channel name" is, although it may be assigned colloquially by peers. The protocol itself does not deal in channel names.

Peers are initially identified by their network address. This would generally be the last 16 bits of their IP address, in hex (4 hex characters). User agents may also expose the peer's MAC address to the user in a "detailed" or "whois"-type display.

Peers may elect to be identified by a nickname, or may choose to keep their automatic identifier (netID). Upon receiving a message from a peer without a known nickname, the receiver may send a QNIK message to the channel. This QNIK message must only be sent after a timer of random duration between 0 & 1 second. This avoids duplicate QNIK messages (although it does not entirely prevent them). The original message sender SHOULD respond with a NICK message to the channel (multicast) indicating their preferred nickname. If the NICK message body is empty, their nickname is unset and their default network-based identifier (above) is used. If a peer sees three QNIK queries for the same IP address and doesn't see a response, it should mark the peer as anonymous and avoid sending any more QNIK queries to that address.

Upon joining a channel, it is customary for a peer to announce its presence via a NICK message to the channel, however it is not mandatory. It is network-efficient to at least send a NICK message before the first MESG to a channel, to avoid clients sending QNIK queries immediately upon receiving a message. The MESG, NICK and QNIK messages can arrive in any order due to the transport layer.

A user's identity should generally presented as a netID-nick pair. This facilitates disambiguation if multiple users select the same nickname. For example the following:

    [52fe] JohnDoe: Hello, comrades.

A message from a peer without a nickname might be presented as:

    [52fe]: Hello, comrades.

User agents SHOULD keep a record of netIDs and ensure that peers that would have the same netID according to the standard rules (ie. the same trailing 16-bits in their IP address) are given unique netIDs. This might involve one of the netIDs being locally mapped to a different randomly-selected netID. In that case the User agents might like to indicate that the netID has been reassigned (eg. [12b5*]) or even to randomly assign all user-visible netIDs based on source IP address and perhaps keep a persistent record of those IDs for subsequent communication). Using the simple low-16-bits algorithm is generally preferable as netIDs will be consistent across implementations/peers.

Possible alternative: use the base64url encoding of the low *three* bytes of the IP address, although it's not as pretty as lowercase hex. Could even be graphical, such as a bitmap based on the address (probably want to hash the address so there isn't a single pixel difference).

A minimalist user agent need only implement the MESG message. This could be useful to communicate automated notifications.

Private messages: Local port 1818. Same as above except that everything that would normally be multicast is sent unicast.

There are no message acknowledgements, and as a UDP protocol there's no built-in indication that a message was received by anyone. However there are some signs that a message was *not* delivered, such as ICMPv6 errors.


## Future considerations

Message acknowledgements. Message IDs to go along with them. C-C-C-Cryptography.

Or just acknowledge (MACK) with a quick hash/crc32 of the message. Probably limit acknowledgements to one-to-one messages.

Message acknowledgements could be implemented by sending a small unicast acknowledgement to the sender. Probably need to introduce an acknowledgement request (ACK-REQ) bit in the message. Also need to define retransmission but that all gets messy and maybe only makes sense for direct messages.

Multiple users per device:
Could assign an IP address per user or have a single "daemon" farm out messages to each user, but message origination in any channels that had both users would be unclear (probably just have to use the default identifier). Or: act as a broker: preface each message with the user's nick/persona. Multi-user-per-IP adds significant complexity.

Broker/relay: Relays messages between subnets. Messages may be prefixed with the src nickname. If broker becomes popular, better to extend protocol with a forwarded message message. Ideal solution is really to use larger scope multicast address, such as site-local, admin-local or organization-local.

Peer-based history. Although against the spirit of "ships passing in the night", querying a peer for the message history could provide useful context when joining a channel.


## Out-of-scope

Spam/ignore/rate-limiting: may be implemented by clients. May use either the IP address or - for link-local - the MAC addresss to ignore a peer. Ignores can be temporary or permanent. Ignores can also apply to specific nicknames, I suppose. User-agents may wish to rate-limit their humans to avoid being auto-ignored by other peers.

Excessive messages from a peer are handled either directly by clients or in the normal network administrative sense. Undesirable behavior may be handled, for example, with a literal slap on the wrist. Peers my also elect to move to a different channel to avoid dealing with undesirables. This is, of course, not an automated procedure and may be negotiated either in-band or, more likely, out-of-band.
