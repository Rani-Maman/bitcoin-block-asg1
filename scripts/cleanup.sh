#!/bin/bash

SHARED_DIR="/tmp/mta_pipes"

echo "Cleaning up MTA Crypto containers and files..."

docker stop $(docker ps -aq --filter "name=mta-") 2>/dev/null || true
docker rm $(docker ps -aq --filter "name=mta-") 2>/dev/null || true

rm -rf ${SHARED_DIR} 2>/dev/null || sudo rm -rf ${SHARED_DIR}
rm -rf /tmp/mta_logs 2>/dev/null || sudo rm -rf /tmp/mta_logs

echo "Cleanup complete!"

echo ""
echo "Remaining containers:"
docker ps --filter "name=mta-" || echo "None"
