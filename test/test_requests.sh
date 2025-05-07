#!/usr/bin/env bash
# test/test_requests.sh

BASE_URL="http://localhost:9090"

echo ">>> Testing deposit success (account 1001, +5000)"
curl -s -X POST "$BASE_URL/deposit" \
     -H "Content-Type: application/json" \
     -d '{"account":1001,"amount":500}'
echo -e "\n\n"

echo ">>> Testing withdraw success (account 1002, -5000)"
curl -s -X POST "$BASE_URL/withdraw" \
     -H "Content-Type: application/json" \
     -d '{"account":1002,"amount":1000}'
echo -e "\n\n"

echo ">>> Testing withdraw insufficient funds (account 1002, -999999999999999)"
curl -s -X POST "$BASE_URL/withdraw" \
     -H "Content-Type: application/json" \
     -d '{"account":1002,"amount":99999}'
echo -e "\n\n"

echo ">>> Testing deposit invalid JSON (missing fields)"
curl -s -X POST "$BASE_URL/deposit" \
     -H "Content-Type: application/json" \
     -d '{"acct":1001,"amt":500}'
echo -e "\n\n"

echo ">>> Testing unknown endpoint"
curl -s -X GET "$BASE_URL/balance"
echo -e "\n"

