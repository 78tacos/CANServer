#!/bin/bash
# Helper script to run the dbcserver container with proper settings
# -p 50123:50123/tcp
# -p 8828:8828/tcp

docker run -it --rm \
    --name dbcserver \
    --network=host \
    --cap-add=NET_RAW \
    -v "$(pwd)/example_server.conf:/app/server.conf:ro" \
    -v "$(pwd)/logs:/app/logs" \
    guyferrari/dbcserver:latest
