# 🏦 Bank Server

> C 언어 기반의 HTTP 은행 서버. 입출금, 잔액 조회, 계좌 이체 등의 API를 지원합니다.
---

### 📌 주요 기능

* **HTTP API 기반 계좌 서비스**

  * 계좌 잔액 조회
  * 입금 / 출금 / 이체 처리
* **MySQL 데이터베이스 연동**

  * 실제 계좌 잔액을 DB에서 조회 및 갱신
  * 트랜잭션 기반의 안전한 입출금 처리
* **스레드풀 기반 동시 처리**

  * 요청당 스레드 생성이 아닌 워커 스레드 모델
  * 비동기 처리로 높은 처리량 확보

---

### 🗂️ 프로젝트 구조

```
├── include/            # 헤더 파일
├── src/                # 주요 C 소스 코드
├── lib/cJSON/          # JSON 파싱 라이브러리
├── nginx/              # NGINX 설정 (JWT 인증 포함)
├── test/               # 테스트 스크립트
├── Makefile            # 빌드 설정
```

<img width="459" height="507" alt="Image" src="https://github.com/user-attachments/assets/06cd77a4-b1bc-4856-9900-ca05fc215fc0" />

---

### 🚀 빌드 및 실행

#### 1. 의존성

* gcc
* pthread
* MySQL 서버
* `mysql_config` CLI
* [cJSON 라이브러리](https://github.com/DaveGamble/cJSON) 포함됨

#### 2. 빌드

```bash
make
```

#### 3. 실행

```bash
./bank_server
```

서버는 기본적으로 포트 `9090`에서 요청을 수신합니다.

---

## 서버 작동 방식

> C 기반 서버에서 네트워크 수신부터 데이터베이스 처리까지의 흐름은 다음과 같습니다.

#### 1. 서버 초기화 (`main.c`)

* `socket()` → `bind()` → `listen()` 을 통해 TCP 소켓 준비
* `signal(SIGINT, handler)` 로 종료 시 리소스 정리
* `account_module_init()`을 통해 MySQL 초기화
* `threadpool_init()` 호출로 워커 스레드 풀 구성

#### 2. 요청 수신 및 처리

```c
int client_fd = accept(listen_fd, NULL, NULL);
threadpool_add_task(client_fd);
```

* 클라이언트 연결 요청이 들어오면 `accept()` 시스템콜로 `client_fd` 수락
* 해당 fd를 작업 큐에 추가 (`threadpool_add_task()`)

#### 3. 스레드풀 구조 (`threadpool.c`)

* 고정된 개수의 **워커 스레드**(기본 4개)가 생성되어 백그라운드에서 `while (1)` 루프 실행
* 스레드는 큐에 요청이 들어올 때까지 `pthread_cond_wait()`로 대기
* 요청이 들어오면 `handle_connection(client_fd)` 호출

#### 4. 워커 스레드 동작

* 각 워커는 독립된 **MySQL 커넥션**을 유지 (`__thread MYSQL *conn`)
* JSON 요청 파싱 → 라우팅 → 비즈니스 로직 처리 → 응답 전송
* 요청 종료 시 `close(client_fd)`로 소켓 닫기

#### 5. HTTP 요청 처리 흐름 (`http.c`)

* 요청 본문 파싱: `Content-Length`, JSON 파싱 등
* URL + 메서드 기반으로 API 분기

  * `/deposit`, `/withdraw`, `/balance`, `/transfer` 등
* 비즈니스 로직 호출 (예: `account_withdraw`, `account_transfer`)
* 응답 본문 작성 후 `write()`로 전송

#### 6. DB 트랜잭션 처리 (`db.c`)

* `START TRANSACTION` → `SELECT ... FOR UPDATE` → `UPDATE` → `COMMIT`
* 동시성 제어를 위해 `FOR UPDATE` 사용
* 모든 작업은 SQL Prepared Statement 기반 (SQL Injection 방지)

---

### 📡 API 명세

> HTTP POST 요청, `Content-Type: application/json`

#### 📥 입금 `/deposit`

```json
{
  "account_number": "1005-001-123456",
  "amount": 500
}
```

#### 📤 출금 `/withdraw`

```json
{
  "account_number": "1005-001-123456",
  "amount": 200
}
```

#### 💰 잔액조회 `/balance`

```json
{
  "account_number": "1005-001-123456"
}
```

#### 🔁 이체 `/deposit`

```json
{
  "sender_account": "1005-001-111111",
  "receiver_account": "1005-001-222222",
  "amount": 1000
}
```

---

### 🧵 스레드풀 구조

* 워커 스레드: 4개 (`POOL_SIZE`)
* 요청 큐: 128개 (`QUEUE_SIZE`)
* 각 워커는 독립적인 DB 커넥션을 사용 (`mysql_thread_init()` / `mysql_thread_end()`)

