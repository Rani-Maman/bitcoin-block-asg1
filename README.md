# MTA Crypto Assignment 3 - Container Implementation

רני ממן, ירדן צור
Rani Maman, Yarden Zur

## Overview

This is a containerized password cracking application where an encrypter and multiple decrypters run in separate Docker containers and communicate via named pipes. The decrypters compete to crack encrypted passwords using brute force techniques.

## Directory Structure

```
Ex03/
├── encrypter/
│   ├── Dockerfile
│   ├── encrypter.c
│   └── Makefile
├── decrypter/
│   ├── Dockerfile
│   ├── decrypter.c
│   └── Makefile
├── scripts/
│   ├── build_images.sh
│   ├── launch.sh
│   └── cleanup.sh
├── shared/
│   └── mtacrypt.conf
├── mta-utils-dev.deb
└── README.md
```

## Prerequisites

1. Docker installed and running
2. Docker Hub account
3. mta-utils-dev.deb file in the root directory

## Setup

### Configure Scripts
```
chmod +x scripts/*.sh
```
(The scripts use my dockerHub username - ranim8. You can switch to yours for checking)

### Build and Push Images
```
docker login
cd scripts
./build_images.sh
```

### Run Application
```
./launch.sh
```

### Monitor Activity
```
docker logs -f mta-encrypter
docker logs -f mta-decrypter-1
docker stats
```

### Cleanup
```
sudo ./cleanup.sh
```

## Docker Hub Images

After building, your images will be:
- your-dockerhub-username/mta-encrypter
- your-dockerhub-username/mta-decrypter

## Configuration

The application reads configuration from /tmp/mta_pipes/mtacrypt.conf:
```
password_length=16
timeout=30
```

## Architecture

### Encrypter Container
- Reads configuration from mounted config file
- Creates main encrypter pipe at /mnt/mta/encrypter_pipe
- Handles decrypter subscriptions
- Generates random printable passwords and encrypts them
- Broadcasts encrypted passwords to all subscribed decrypters
- Validates decrypted password attempts
- Logs activity to /var/log/encrypter.log

### Decrypter Container
- Finds next available decrypter ID
- Creates own pipe: /mnt/mta/decrypter_pipe_N
- Sends subscription request to encrypter
- Receives encrypted passwords via named pipes
- Attempts decryption using random keys
- Sends successful decryptions back to encrypter
- Logs activity to /var/log/decrypter_N.log

## Communication Protocol

### Subscription Message
```
SUBSCRIBE:decrypter_pipe_N
```

### Password Broadcast
```
PASSWORD:password_id:encrypted_data
```

### Decrypted Password Submission
```
DECRYPTED:decrypter_id:plain_password
```

## Expected Behavior

Encrypter Logs:
```
[SERVER] [INFO] Config loaded: password_length=16, timeout=30
[SERVER] [INFO] New password generated: A7x#mK9$pLq2sT4v, key: fG8hJ1kL...
[SERVER] [INFO] Decrypter added: /mnt/mta/decrypter_pipe_1 (total: 1)
[SERVER] [OK] Password decrypted successfully by client 1...
```

Decrypter Logs:
```
[CLIENT #1] [INFO] Created pipe: /mnt/mta/decrypter_pipe_1
[CLIENT #1] [INFO] Subscription sent
[CLIENT #1] [INFO] Received new password (ID: 1): [encrypted_data]
[CLIENT #1] [INFO] After decryption(...), sending to server after 15847 iterations
```

## Troubleshooting

- Make sure mta-utils-dev.deb is in the root directory
- Verify you're logged into Docker Hub: docker login
- Use sudo ./cleanup.sh for cleanup
- Check containers are running: docker ps --filter "name=mta-"

## Requirements

- Two separate Docker images (encrypter/decrypter)
- Named pipe communication via mounted directory
- Subscription/broadcast protocol
- Configuration file reading
- Proper logging to /var/log
- Non-blocking pipe operations
- Encrypter starts before decrypters
- Dynamic decrypter ID assignment
