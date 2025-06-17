#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char *argv[]){
    int flag = 1;
    int sum = 0;
    int status = -1;
    pid_t pid = -1;
    char response = '\0';
    int *arr = NULL;
    int n = 0;
    int index = 0;
    char buf[BUFSIZ];
    char temp[BUFSIZ];
    ssize_t size = 0;
    while(1){
        printf("입력을 그만하고 싶으면 q를 입력하세요.\n");
        while(1){
            printf("숫자를 입력하세요. : \n");
            size = read(0, temp, BUFSIZ);
            if(size == -1){
                perror("read error");
                exit(1);
            }
            // 개행 문자 를 널 문자로 변경
            temp[size - 1] = '\0'; 
            // q 또는 Q 입력 시 종료
            if (temp[0]=='q'|| temp[0]=='Q'){
                break;
            }
            // 읽은 문자열로 표현된 숫자를 buf에 저장
            strcat(buf, temp);
            // 각 숫자는 공복으로 구분
            // 추후에 strtok으로 공백을 기준으로 분리하기 위함
            strcat(buf, " ");
        }

        // 숫자 개수 세기
        // strtok사용하면 원본이 변형되므로
        // temp에 buf를 복사한 후 사용
        strcpy(temp, buf);
        char *token = strtok(temp, " ");
        while(token != NULL){
            n++;
            token = strtok(NULL, " ");
        }
        
        // 숫자를 입력했는지 검증
        if (n == 0) {
            printf("입력된 숫자가 없습니다.\n");
            continue;
        }

        // 숫자 개수 크기의 int형 배열 동적 할당
        arr = (int *)malloc(n * sizeof(int));
        if(arr == NULL){
            perror("malloc error");
            exit(1);
        }

        // buf에 저장된 숫자들을 공백을 기준으로 분리하여 int형 배열에 저장
        token = strtok(buf, " ");
        while(token != NULL){
            arr[index++] = atoi(token);
            token = strtok(NULL, " ");
        }

        // 자식 프로세스 생성
        switch (pid = fork()){
            // 자식 프로세스 생성 실패
            case -1:
                perror("fork error");
                exit(1);
            // 자식 프로세스인 경우
            // 정수형 배열의 합을 계사한 후 출력후 종료
            case 0:
                for(int i = 0; i < n; i++){
                    sum += arr[i];
                }
                printf("Child process: Sum = %d\n", sum);
                free(arr);
                exit(0);
            // 부모 프로세스인 경우
            // 자식 프로세스가 종료될 때까지 대기
            default:
                wait(&status);
                // 자식 프로세스가 정상적으로 종료되었는지 확인
                if(status == -1){
                    perror("wait error");
                    exit(1);
                }
                // 다시 숫자를 입력받고 계산할 것인지 물어봄
                while(flag){
                    printf("try again (Y / N) :");
                    scanf("%c", &response);
                    getchar(); // 버퍼에 남아있는 개행 문자 제거
                    if (response == 'N' || response == 'n') {
                        printf("프로그램 종료.\n");
                        exit(0);
                    }
                    else if (response == 'Y' || response == 'y') {
                        // 위의 동작을 반복하기 위해 초기화
                        flag = 0;
                        n=0;
                        sum = 0;
                        buf[0] = '\0';
                        index = 0;
                    } else {
                        printf("잘못된 입력입니다. Y 또는 N을 입력하세요.\n");
                    }
                    
                }
                flag = 1;
        }
    }
    return 0;
}