FROM gcc:13 AS builder

RUN apt-get update && apt-get install -y cmake git && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy project files needed for the tests
COPY config.h /app/config.h
COPY credential_store.h /app/credential_store.h
COPY credential_store.cpp /app/credential_store.cpp
COPY health_client.h /app/health_client.h
COPY health_client.cpp /app/health_client.cpp
COPY test/ /app/test/

WORKDIR /app/test

# Configure and build
RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build --parallel

# Run all tests
CMD ["ctest", "--test-dir", "build", "--output-on-failure"]
