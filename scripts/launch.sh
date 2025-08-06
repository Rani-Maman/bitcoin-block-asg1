#!/bin/bash

DOCKER_USER="ranim8"
NUM_DECRYPTERS=3
SHARED_DIR="/tmp/mta_pipes"

if [ "$DOCKER_USER" != "ranim8" ]; then
    echo "Error: Update DOCKER_USER in launch.sh with the right username"
    exit 1
fi

echo "Starting MTA Crypto containers..."

mkdir -p ${SHARED_DIR}
echo "password_length=16" > ${SHARED_DIR}/mtacrypt.conf
echo "timeout=30" >> ${SHARED_DIR}/mtacrypt.conf

docker stop $(docker ps -aq --filter "name=mta-") 2>/dev/null || true
docker rm $(docker ps -aq --filter "name=mta-") 2>/dev/null || true

echo "Starting encrypter container..."
docker run -d --name mta-encrypter \
    -v ${SHARED_DIR}:/mnt/mta \
    -v /tmp/mta_logs:/var/log \
    ${DOCKER_USER}/mta-encrypter

sleep 3

echo "Starting ${NUM_DECRYPTERS} decrypter containers..."
for i in $(seq 1 ${NUM_DECRYPTERS}); do
    docker run -d --name mta-decrypter-${i} \
        -v ${SHARED_DIR}:/mnt/mta \
        -v /tmp/mta_logs:/var/log \
        ${DOCKER_USER}/mta-decrypter
    echo "Started decrypter ${i}"
    sleep 1
done

echo ""
echo "Containers started successfully!"
echo ""
echo "Running containers:"
docker ps --filter "name=mta-"

echo ""
echo "To view logs:"
echo "  Encrypter: docker logs -f mta-encrypter"
echo "  Decrypter 1: docker logs -f mta-decrypter-1"
echo ""
echo "To stop all: sudo ./cleanup.sh"
