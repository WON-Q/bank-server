// include/http.h
#ifndef HTTP_H
#define HTTP_H

// 클라이언트 소켓(fd)를 받아서
// 1) 요청 파싱 → 2) 라우팅 → 3) JSON 처리 → 4) 응답 전송 → 5) close(fd)
// 단일 연결 처리 함수
void handle_connection(int client_fd);

#endif // HTTP_H

