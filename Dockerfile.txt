FROM ubuntu:24.04

RUN apt-get update && \
    apt-get install -y g++ && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY backend ./backend
COPY frontend ./frontend

RUN g++ -std=c++20 backend/main.cpp -o matrix_server -pthread

EXPOSE 10000

CMD ["./matrix_server"]