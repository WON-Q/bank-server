# 멀티 스테이지 빌드를 사용하여 최종 이미지 크기를 줄입니다
FROM ubuntu:22.04 AS builder

# 필요한 패키지 설치
RUN apt-get update && apt-get install -y \
    gcc \
    make \
    libmysqlclient-dev \
    mysql-client \
    pkg-config \
    git \
    && rm -rf /var/lib/apt/lists/*

# 작업 디렉토리 설정
WORKDIR /app

# 소스 코드 복사
COPY . .

# cJSON 서브모듈이 없는 경우를 대비해 직접 다운로드
RUN if [ ! -f lib/cJSON/cJSON.c ]; then \
    mkdir -p lib/cJSON && \
    git clone https://github.com/DaveGamble/cJSON.git /tmp/cJSON && \
    cp /tmp/cJSON/cJSON.c /tmp/cJSON/cJSON.h lib/cJSON/ && \
    rm -rf /tmp/cJSON; \
    fi

# 빌드
RUN make clean && make

# 실행용 경량 이미지
FROM ubuntu:22.04

# 런타임에 필요한 패키지만 설치
RUN apt-get update && apt-get install -y \
    libmysqlclient21 \
    && rm -rf /var/lib/apt/lists/*

# 실행용 사용자 생성 (보안을 위해)
RUN useradd -r -s /bin/false bankuser

# 작업 디렉토리 설정
WORKDIR /app

# 빌드된 바이너리만 복사
COPY --from=builder /app/bank_server .

# 권한 설정
RUN chown bankuser:bankuser bank_server

# 포트 노출
EXPOSE 9090

# 사용자 변경
USER bankuser

# 실행
CMD ["./bank_server"]