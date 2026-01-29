# Build stage
FROM gcc:13 AS builder

WORKDIR /app

COPY *.cpp *.hpp *.h ./

RUN g++ -std=c++17 -O3 -Wall -static -o gcd_server main.cpp -pthread

# Runtime stage
FROM debian:bookworm-slim

RUN useradd -r -s /bin/false appuser

WORKDIR /app

COPY --from=builder /app/gcd_server .

RUN chown appuser:appuser gcd_server

USER appuser

EXPOSE 8080

CMD ["./gcd_server", "8080"]
