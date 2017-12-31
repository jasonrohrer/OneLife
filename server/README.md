# Get your own server up and running

## Setup
In this directory:
```
./configure
make

ln -s ../OneLifeData7/transitions .
ln -s ../OneLifeData7/objects .
ln -s ../OneLifeData7/categories .

```
Modify `settings/`:
1. `echo 0 > requreTicketServerCheck.ini`
2. `echo 0 > requireClientPassword.ini`


In your client, modify `settings/`:
1. `echo 1 > useCustomServer.ini`

## Run in either `/server/` and/or `/OneLife/` as needed:
```
./OneLifeServer
./OneLifeApp
```

You can also use the run script if you'd like it to be nohup'd.
