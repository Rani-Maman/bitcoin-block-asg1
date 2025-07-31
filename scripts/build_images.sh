#!/bin/bash

DOCKER_USER="ranim8"

if [ "$DOCKER_USER" != "ranim8" ]; then
    echo "Error: Update DOCKER_USER in build_images.sh with the right username"
    exit 1
fi

echo "Building MTA Crypto Docker images..."

cp ../mta-utils-dev.deb ../encrypter/
cp ../mta-utils-dev.deb ../decrypter/

docker build -t ${DOCKER_USER}/mta-encrypter ../encrypter/
docker build -t ${DOCKER_USER}/mta-decrypter ../decrypter/

echo "Pushing images to Docker Hub..."
docker push ${DOCKER_USER}/mta-encrypter
docker push ${DOCKER_USER}/mta-decrypter

rm ../encrypter/mta-utils-dev.deb
rm ../decrypter/mta-utils-dev.deb

echo "Build complete!"
echo "Images:"
echo "  ${DOCKER_USER}/mta-encrypter"
echo "  ${DOCKER_USER}/mta-decrypter"
