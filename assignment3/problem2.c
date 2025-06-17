#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>

#define BUF_SIZE 4096
// 리눅스 기본 페이지 크기(4KB) 기준 버퍼 크기

int child_fd1, child_fd2;
// atexit 사용하기 위해 전역 변수로 선언
void cleanup() {
    close(child_fd1);
    close(child_fd2);
}
// 자식 프로세스 종료 시 파이프 닫기

int main() {
    char buf[BUF_SIZE];
    pid_t pid;
    int n = 0;
    char c;
    const char prompt_p1[] = "[프로세스 1] 입력 (끝내고 싶으면 .입력): ";
    const char prompt_p2[] = "[프로세스 2] 입력 (끝내고 싶으면 .입력): ";
    const char msg_recv_p1[] = "[프로세스 1] 프로세스 2로부터 받은 입력: ";
    const char msg_recv_p2[] = "[프로세스 2] 프로세스 1로부터 받은 입력: ";
    const char msg_exit_p1[] = "[프로세스 1] 종료합니다.\n";
    const char msg_exit_p2[] = "[프로세스 2] 종료합니다.\n";

    if (mkfifo("fifo1", 0666) == -1) perror("mkfifo");
    if (mkfifo("fifo2", 0666) == -1) perror("mkfifo");
    // FIFO 파일 생성


    switch (pid = fork()) {
    // 자식 프로세스 생성
        case -1:
            perror("fork");
            exit(1);

        // 에러처리: fork 실패 시 프로그램 종료
        case 0: {
            // Child Process : 프로세스 2
            child_fd1 = open("fifo1", O_RDONLY);
            child_fd2 = open("fifo2", O_WRONLY);
            // 자식 프로세스가 사용할 FIFO 파일을 읽기 전용, 쓰기 전용으로 열기
            if (child_fd1 < 0 || child_fd2 < 0) {
                perror("open");
                exit(1);
            }
            // 에러처리: FIFO 파일 열기 실패 시 프로그램 종료
            atexit(cleanup);
            // 자식 프로세스 종료 시 파이프 닫기

            while (1) {
                int n = read(child_fd1, buf, BUF_SIZE);
                buf[n] = '\0';
                // 프로세스 1로부터 입력을 읽음
                if (n <= 0) {
                    perror("read");
                    exit(1);
                    // 에러처리: read 실패 시 프로그램 종료
                }
                                   
                write(1, msg_recv_p2, strlen(msg_recv_p2));
                write(1, buf, strlen(buf));
                write(1, "\n", 1);
                // 프로세스 1로부터 받은 입력을 출력

                if (strcmp(buf, ".") == 0){
                    write(1, msg_exit_p2, strlen(msg_exit_p2));
                    break;
                }
                // 프로세스 1이 종료 요청을 보낸 경우
                    

                write(1, prompt_p2, strlen(prompt_p2));

                n=0;
                // 입력 버퍼 초기화
                while (1) {
                    if (read(0, &c, 1)<= 0) {
                        perror("read");
                        exit(1);
                        // 에러처리: read 실패 시 프로그램 종료
                    }
                    if (c == '\n') {
                        continue;
                    }
                    // 개행 문자는 무시
                    buf[n++] = c;
                    // 입력 버퍼에 문자를 추가
                    if (c == '.') {
                        break;
                    }
                    // 입력이 '.'인 경우 루프 종료
                }
                buf[n] = '\0';
                // 입력 버퍼의 끝에 널 문자 추가

                write(child_fd2, buf, strlen(buf));
                // 프로세스 1로 입력을 전송

                if (strcmp(buf, ".") == 0) {
                    write(1, msg_exit_p2, strlen(msg_exit_p2));
                    break;
                }
                // 프로세스 2가 종료 요청을 보낸 경우
                
            }

            pause();
            // 부모 프로세스가 KILL을 보내기 전까지 대기
            break;
        }

        default: {
            // Parent Process : 프로세스 1
            int parent_fd1, parent_fd2;
            parent_fd1 = open("fifo1", O_WRONLY);
            parent_fd2 = open("fifo2", O_RDONLY);
            // 부모 프로세스가 사용할 FIFO 파일을 쓰기 전용, 읽기 전용으로 열기
            if (parent_fd1 < 0 || parent_fd2 < 0) {
                perror("open");
                exit(1);
            }
            // 에러처리: FIFO 파일 열기 실패 시 프로그램 종료

            while (1) {
                write(1, prompt_p1, strlen(prompt_p1));

                n=0;
                // 입력 버퍼 초기화
                while (1) {
                    if (read(0, &c, 1)<= 0) {
                        perror("read");
                        exit(1);
                        // 에러처리: read 실패 시 프로그램 종료
                    }
                    if (c == '\n') {
                        continue;
                    }
                    // 개행 문자는 무시
                    buf[n++] = c;
                    // 입력 버퍼에 문자를 추가
                    if (c == '.') {
                        break;
                    }
                    // 입력이 '.'인 경우 루프 종료
                }
                buf[n] = '\0';
                // 입력 버퍼의 끝에 널 문자 추가

               

                write(parent_fd1, buf, strlen(buf));
                // 프로세스 2로 입력을 전송
                if (strcmp(buf, ".") == 0){
                    write(1, msg_exit_p1, strlen(msg_exit_p1));
                    sleep(1); 
                    break;
                }
                // 프로세스 1이 종료 요청을 보낸 경우
                // 프로세스 2가 입력을 받을 때까지 대기
                    

                n = read(parent_fd2, buf, BUF_SIZE);
                buf[n] = '\0';
                // 프로세스 2로부터 입력을 받음
                if (n < 0) {
                    perror("read");
                    exit(1);
                    // 에러처리: read 실패 시 프로그램 종료
                }
         
                write(1, msg_recv_p1, strlen(msg_recv_p1));
                write(1, buf, strlen(buf));
                write(1, "\n", 1);
                // 프로세스 2로부터 받은 입력을 출력

                if (strcmp(buf, ".") == 0) {
                    write(1, msg_exit_p1, strlen(msg_exit_p1));
                    break;
                }
                // 프로세스 2가 종료 요청을 보낸 경우
                
                
            }
            kill(pid, SIGTERM);  // 자식에게 종료 요청
            wait(NULL);
            close(parent_fd1);
            close(parent_fd2);
            // 부모 프로세스에서 사용한 파이프 닫음
            break;
        }
    }

    return 0;
}
