# Commands

Commands can be typed inside the chat window and always start with a `/`. Using no slash will be interpreted as "sending a message" thus it will send a message to the server.

## List of commands

Change name:
`/setname [name]`

Create a server:
`/host [name]`

Join a server:
`/join [ip] [name]`

Leave the server:
`/leave`

### Notes

- All names must be between 3 (inclusive) and 32 (exclusive) characters long.
- An IP usually has the form: `xxx.xxx.xxx.xxx`
- A user can host as many servers as they want

## Example

### Server

```
/host hut
Now hosting server `hut`!
The password is ...
```
```
'steve' has connected to the server!
```

### User

```
/setname steve
/join 127.0.0.1 hut
Successfully joined server 'hut'!
```

