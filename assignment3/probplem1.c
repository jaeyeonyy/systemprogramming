#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <limits.h>

int fd1[2], fd2[2];
// fd1: 부모 → 자식 데이터 전송용 파이프
// fd2: 자식 → 부모 데이터 전송용 파이프
// atexit 사용하기 위해 전역 변수로 선언

void cleanup() {
    close(fd1[0]);
    close(fd2[1]);
}
// 자식 프로세스 종료 시 파이프 닫기

int main() {

    pid_t pid;
    if (pipe(fd1) == -1) {
        perror("pipe");
        exit(1);
    }

    if (pipe(fd2) == -1) {
        perror("pipe");
        exit(1);
    }
    // 에러처리: 파이프 생성 실패 시 프로그램 종료

    
    switch (pid = fork()) {
    // 자식 프로세스 생성
        case -1 :
            perror("fork");
            exit(1);

        // 에러처리: fork 실패 시 프로그램 종료
        case 0 :    // 자식 프로세스
            close(fd1[1]);
            close(fd2[0]);
            // 자식 프로세스에서 안 쓰는 파이프의 끝을 닫음
            atexit(cleanup);
            // 자식 프로세스 종료 시 파이프 닫기

            char path_child[PATH_MAX];
            // 부모가 보낸 디렉토리 경로를 저장할 변수
            // PATH_MAX는 limit.h에서 정의된 최대 경로 길이
            int file_count = 0;
            // 총 파일 개수 카운팅
            char str_num[20];
            // 파일 개수를 문자열로 저장할 변수
            while (1) {
                int n_child = read(fd1[0], path_child, PATH_MAX);
                // 부모로부터 디렉토리 경로를 읽어옴
                if (n_child <= 0){
                    perror("read");
                    exit(1);
                    // 에러처리: read 실패 시 프로그램 종료
                    
                }
                if (strcmp(path_child, "//") == 0) {
                    // 부모가 종료 요청을 보낸 경우
                    break;
                }

                struct dirent *entry;
                DIR *dir = opendir(path_child);
                if (dir == NULL) {
                    perror("opendir");
                    continue;
                    // 에러처리: 디렉토리 열기 실패 시 경고 메시지 출력 후 다음 반복
                }
                while ((entry = readdir(dir)) != NULL) {
                    // 디렉토리 안 모든 항목 순회
                    if (entry->d_type == DT_REG) {
                        file_count++;
                        // 일반 파일인 경우 카운트 증가
                    }
                }
                closedir(dir);
                // 파일 카운트 후 디렉토리 닫기
                
            }
            sprintf(str_num, "%d", file_count);
            // 최종 파일 개수를 문자열로 변환
            write(fd2[1], str_num, strlen(str_num));
            // 부모에게 파일 개수를 전송
            pause();
            //
        default :   // 부모 프로세스
            close(fd1[0]);
            close(fd2[1]);
            // 부모 프로세스에서 안 쓰는 파이프의 끝을 닫음

            char path_parent[PATH_MAX];
            // 부모가 입력할 디렉토리 경로를 저장할 변수
            // PATH_MAX는 limit.h에서 정의된 최대 경로 길이
            char * prompt = "디렉토리를 입력해주세요 ('//' 입력 시 종료): ";
            char file_total[20];
            // 파일 개수를 저장할 변수
            int n_parent = 0;
            while (1) {
                write(1, prompt, strlen(prompt));
                n_parent = read(0, path_parent, PATH_MAX);
                // 표준 입력에서 디렉토리 경로를 읽어옴
                if (n_parent <= 0) {
                    perror("read");
                    exit(1);
                    // 에러처리: read 실패 시 프로그램 종료
                }
                path_parent[n_parent-1] = '\0';
                // 개행문자를 널 문자로 변경
                write(fd1[1], path_parent, strlen(path_parent) + 1);
                // 자식 프로세스에 디렉토리 경로 전송
                if (strcmp(path_parent, "//") == 0) {
                    break;
                }
            }
           
            n_parent = read(fd2[0], &file_total, sizeof(file_total));
            // 자식 프로세스로부터 파일 개수를 읽어옴
            if (n_parent <= 0) {
                perror("read");
                exit(1);
                // 에러처리: read 실패 시 프로그램 종료                
            }
            char *end_ptr="파일 개수: ";
            write(1, end_ptr, strlen(end_ptr));
            write(1, file_total, n_parent);
            write(1, "\n", 1);
            // 표준 출력에 파일 개수 출력
            kill(pid, SIGTERM);  // 자식에게 종료 요청
            wait(NULL);
            close(fd1[1]);
            close(fd2[0]);
            // 부모 프로세스에서 안 쓰는 파이프의 끝을 닫음

            // 자식 프로세스가 종료될 때까지 대기
            break;
    }
}
