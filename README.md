# OneLife Server

This repository enables the OHOL Server to be run on a docker container locally.

## Requirements

1. docker (https://www.docker.com/products/docker-desktop/ or https://rancherdesktop.io/)

## Instructions

1. Clone this repository

`git clone https://github.com/thaulos-bolsens/OneLifeServer.git`

`cd OneLifeServer`

2. Start server

`docker compose up -d`

3. Stop Server

`docker compose stop`

4. Rebuild server (useful when a newer version comes out)

`docker rm ohol-server`

`docker rmi onelifeserver_server`

`docker compose up -d`

5. Starting map/etc from scratch

`docker rm ohol-server`

`docker rmi onelifeserver_server`

delete files under `./docker/data`

`docker compose up -d`

## Persistence

Data is persisted even if the container and images are removed. You can find the sqlite files in `./docker/data`
