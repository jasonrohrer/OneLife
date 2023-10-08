FROM ubuntu:22.04

RUN apt-get update -y && apt-get upgrade -y
RUN apt install git make build-essential sqlite3 libsqlite3-dev -y

WORKDIR /ohol

COPY ./server/installYourOwnServer/serverPullAndBuildLinux.sh ./serverPullAndBuildLinux.sh

RUN bash serverPullAndBuildLinux.sh

WORKDIR /ohol/OneLife/server

RUN mkdir /ohol/data
RUN touch /ohol/data/biome.db && touch /ohol/data/curseCount.db && touch /ohol/data/curses.db && touch /ohol/data/eve.db && touch /ohol/data/floor.db && touch /ohol/data/floorTime.db && touch /ohol/data/grave.db && \
    touch /ohol/data/lookTime.db && touch /ohol/data/map.db && touch /ohol/data/mapTime.db && touch /ohol/data/meta.db && touch /ohol/data/playerStats.db && touch /ohol/data/trust.db

RUN ln -s /ohol/data/biome.db biome.db
RUN ln -s /ohol/data/curseCount.db curseCount.db
RUN ln -s /ohol/data/curses.db curses.db
RUN ln -s /ohol/data/eve.db eve.db
RUN ln -s /ohol/data/floor.db floor.db
RUN ln -s /ohol/data/floorTime.db floorTime.db
RUN ln -s /ohol/data/grave.db grave.db
RUN ln -s /ohol/data/lookTime.db lookTime.db
RUN ln -s /ohol/data/map.db map.db
RUN ln -s /ohol/data/mapTime.db mapTime.db
RUN ln -s /ohol/data/meta.db meta.db
RUN ln -s /ohol/data/playerStats.db playerStats.db
RUN ln -s /ohol/data/trust.db trust.db

EXPOSE 8005

#ENTRYPOINT ["tail", "-f", "/dev/null"]
ENTRYPOINT  ["./OneLifeServer"]
