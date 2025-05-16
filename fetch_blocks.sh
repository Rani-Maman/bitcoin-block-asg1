#!/bin/bash

# Check input
if [ -z "$1" ]; then
    echo "Usage: $0 <number_of_blocks>"
    exit 1
fi

NUM_BLOCKS="$1"

# Check if input is a number
if ! [[ "$NUM_BLOCKS" =~ ^[0-9]+$ ]] || [ "$NUM_BLOCKS" -le 0 ]; then
    echo "Error: Please enter a positive integer."
    exit 1
fi

# Using wget to fetch the latest blockchain info
CHAIN_DATA=$(wget -qO- https://api.blockcypher.com/v1/btc/main)
LATEST_HEIGHT=$(echo "$CHAIN_DATA" | grep -oP '"height":\s*\K[0-9]+')

START_HEIGHT=$((LATEST_HEIGHT - NUM_BLOCKS + 1))
if [ "$START_HEIGHT" -lt 0 ]; then
    START_HEIGHT=0
fi

OUTPUT="blocks.dat"
> "$OUTPUT"

echo "Fetching blocks $START_HEIGHT to $LATEST_HEIGHT..."

for (( i=START_HEIGHT; i<=LATEST_HEIGHT; i++ )); do
    echo "Fetching block height $i..."

    BLOCK_JSON=$(wget -qO- "https://api.blockcypher.com/v1/btc/main/blocks/$i")

    HASH=$(echo "$BLOCK_JSON" | grep -oP '"hash":\s*"\K[^"]+')
    HEIGHT=$(echo "$BLOCK_JSON" | grep -oP '"height":\s*\K[0-9]+')
    TOTAL=$(echo "$BLOCK_JSON" | grep -oP '"total":\s*\K[0-9]+')
    TIME=$(echo "$BLOCK_JSON" | grep -oP '"time":\s*"\K[^"]+')
    RELAYED=$(echo "$BLOCK_JSON" | grep -oP '"relayed_by":\s*"\K[^"]*')
    PREV=$(echo "$BLOCK_JSON" | grep -oP '"prev_block":\s*"\K[^"]*')

    echo "${HASH}|${HEIGHT}|${TOTAL}|${TIME}|${RELAYED}|${PREV}" >> "$OUTPUT"
done

echo "Saved $NUM_BLOCKS blocks to $OUTPUT"

