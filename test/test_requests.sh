#!/usr/bin/env bash
# test/test_requests.sh

BASE_URL="http://localhost:9090"
TEST_ACCOUNT="1005-001-123456"

start_server() {
  ./bank_server & SERVER_PID=$!
  # 서버 기동 대기
  sleep 1
}

stop_server() {
  kill $SERVER_PID
  wait $SERVER_PID 2>/dev/null
}

echo "▶▶▶ Deposit +500"
start_server
curl -i -X POST "$BASE_URL/deposit" \
     -H "Content-Type: application/json" \
     -d "{
       \"account_number\":\"$TEST_ACCOUNT\",
       \"amount\":500
     }"
echo -e "\n"
stop_server

echo "▶▶▶ Withdraw -200"
start_server
curl -i -X POST "$BASE_URL/withdraw" \
     -H "Content-Type: application/json" \
     -d "{
       \"account_number\":\"$TEST_ACCOUNT\",
       \"amount\":200
     }"
echo -e "\n"
stop_server

echo "▶▶▶ Balance Check"
start_server
curl -i -X POST "$BASE_URL/balance" \
     -H "Content-Type: application/json" \
     -d "{
       \"account_number\":\"$TEST_ACCOUNT\"
     }"
echo -e "\n"
stop_server

echo "▶▶▶ Withdraw Insufficient Funds"
start_server
curl -i -X POST "$BASE_URL/withdraw" \
     -H "Content-Type: application/json" \
     -d "{
       \"account_number\":\"$TEST_ACCOUNT\",
       \"amount\":999999999
     }"
echo -e "\n"
stop_server

echo "▶▶▶ Invalid JSON (missing fields)"
start_server
curl -i -X POST "$BASE_URL/deposit" \
     -H "Content-Type: application/json" \
     -d '{
       "acct_num":"'"$TEST_ACCOUNT"'",
       "amt":500
     }'
echo -e "\n"
stop_server

echo "▶▶▶ Unknown Endpoint"
start_server
curl -i -X GET "$BASE_URL/unknown"
echo -e "\n"
stop_server

