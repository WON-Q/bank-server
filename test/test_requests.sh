#!/usr/bin/env bash
# test/test_requests.sh

BASE_URL="http://localhost:9090"

start_server() {
  ./bank_server & SERVER_PID=$!
  # 서버가 완전히 실행될 때까지 sleep 적용
  sleep 1
}

stop_server() {
  kill $SERVER_PID
  wait $SERVER_PID 2>/dev/null
}

# 1) Deposit +500
start_server
echo ">>> Testing deposit success (account 1001, +500)"
curl -s -X POST "$BASE_URL/deposit" \
     -H "Content-Type: application/json" \
     -d '{"account":1001,"amount":500}'
echo -e "\n"
stop_server

# 2) Withdraw -1000
start_server
echo ">>> Testing withdraw success (account 1002, -1000)"
curl -s -X POST "$BASE_URL/withdraw" \
     -H "Content-Type: application/json" \
     -d '{"account":1002,"amount":1000}'
echo -e "\n"
stop_server

# 3) Withdraw insufficient funds
start_server
echo ">>> Testing withdraw insufficient funds (account 1002, -999999999999999)"
curl -s -X POST "$BASE_URL/withdraw" \
     -H "Content-Type: application/json" \
     -d '{"account":1002,"amount":999999999999999}'
echo -e "\n"
stop_server

# 4) Invalid JSON
start_server
echo ">>> Testing deposit invalid JSON (missing fields)"
curl -s -X POST "$BASE_URL/deposit" \
     -H "Content-Type: application/json" \
     -d '{"acct":1001,"amt":500}'
echo -e "\n"
stop_server

# 5) Unknown endpoint
start_server
echo ">>> Testing unknown endpoint"
curl -s -X GET "$BASE_URL/balance"
echo -e "\n"
stop_server

