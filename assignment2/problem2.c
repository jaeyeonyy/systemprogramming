#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/types.h>

int main(int argc, char *argv[]){
    
    char str[BUFSIZ];
    ssize_t size = -1;
    // 4칙 연산 수식 입력받기
    size = read(0, str, BUFSIZ);
    if(size == -1){
        perror("read error");
        exit(1);
    }
    str[size - 1] = '\0';

    // input.txt 파일에 입력받은 수식 저장
    int fd = open("input.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if(fd == -1){
        perror("open error");
        exit(1);
    }
    ssize_t write_size = write(fd, str, size);
    if(write_size == -1){
        perror("write error");
        exit(1);
    }
    close(fd);

    // 자식 프로세스 생성
    pid_t pid = fork();
    switch(pid){
        // 자식 프로세스 생성 실패
        case -1:
            perror("fork error");
            exit(1);
        // 자식 프로세스인 경우
        case 0:
            // calculator.out 실행
            // input.txt 파일명을 인자로 전달
            execl("./calculator.out", "./calculator.out", "input.txt", NULL);
            perror("execl error");
            exit(1);
        // 부모 프로세스인 경우
        default: 
            // 자식 프로세스 종료 대기
            wait(NULL); 
    }

    return 0;

}